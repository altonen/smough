#include <fs/binfmt.h>
#include <fs/char.h>
#include <fs/dentry.h>
#include <fs/devfs.h>
#include <fs/fs.h>
#include <fs/initramfs.h>
#include <lib/hashmap.h>
#include <lib/list.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/kpanic.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <errno.h>
#include <stdbool.h>

#define NUM_FS_TYPES 2
#define NUM_FS 1

static mm_cache_t *path_cache;
static mm_cache_t *fs_ctx_cache;
static mm_cache_t *file_ctx_cache;

static list_head_t mountpoints;
static list_head_t superblocks;

static mount_t *root_fs;
static hashmap_t *fs_types;

// array of "static" file systems (known at compile time)
// that kernel needs in order to work. Keeping them here
// makes the initialization code much cleaner.
// (same thing with the mountpoint array below)
static fs_type_t fs_arr[NUM_FS_TYPES] = {
    {
        .fs_name = "devfs",
        .get_sb  = devfs_get_sb,
        .kill_sb = devfs_kill_sb
    },
    {
        .fs_name = "initramfs",
        .get_sb  = initramfs_get_sb,
        .kill_sb = initramfs_kill_sb
    }
};

static struct {
    char *type;
    char *target;
    dentry_t *mount;
} file_systems[NUM_FS] = {
    {
        .type   = "devfs",
        .target = "dev",
        .mount  = NULL
    },
};

static mount_t *alloc_empty_mount(void)
{
    mount_t *mnt = NULL;

    if (!(mnt = kmalloc(sizeof(mount_t))))
        return NULL;

    mnt->mnt_sb      = NULL;
    mnt->mnt_root    = NULL;
    mnt->mnt_devname = NULL;
    mnt->mnt_type    = NULL;
    mnt->mnt_mount   = NULL;

    list_init(&mnt->mnt_list);

    return mnt;
}

static int vfs_mount_pseudo(char *target, char *type, dentry_t *mountpoint)
{
    mount_t *mnt  = NULL;
    fs_type_t *fs = NULL;

    if (!target || !type || !mountpoint)
        return -EINVAL;

    if (!(fs = hm_get(fs_types, (char *)type))) {
        kprint("vfs - filesystem type %s has not been registered!\n", type);
        return -ENOSYS;
    }

    if (!(mnt = alloc_empty_mount())) {
        kprint("vfs - failed to allocate mountpoint for %s\n", type);
        return -ENOMEM;
    }

    mnt->mnt_sb      = fs->get_sb(fs, NULL, 0, NULL);
    mnt->mnt_type    = type;
    mnt->mnt_devname = kstrdup(type);
    mnt->mnt_mount   = mountpoint;
    mnt->mnt_root    = mnt->mnt_sb->s_root;

    mountpoint->d_count++;

    list_append(&mountpoints, &mnt->mnt_list);

    return 0;
}

void vfs_init(void)
{
    fs_types   = hm_alloc_hashmap(16, HM_KEY_TYPE_STR);
    path_cache = mm_cache_create(sizeof(path_t));

    list_init(&mountpoints);
    list_init(&superblocks);

    dentry_init();
    inode_init();
    file_init();
    binfmt_init();

    // register all known filesystems that might be needed
    for (int i = 0; i < NUM_FS_TYPES; ++i) {
        if (vfs_register_fs(&fs_arr[i]) < 0)
            kprint("vfs - failed to register %s\n", fs_arr[i].fs_name);
    }

    // create empty root
    root_fs            = alloc_empty_mount();
    root_fs->mnt_mount = dentry_alloc_orphan("/", T_IFDIR);
    root_fs->mnt_type  = "rootfs";

    list_append(&mountpoints, &root_fs->mnt_list);

    // mount the pseudo filesystems, we must use vfs_mount_pseudo()
    // to mount these special file systems which skips most of the error
    // checks and path look ups.
    //
    // This is done because scheduler hasn't been initialized and
    // `vfs_path_lookup()` uses "current" to determine the path.
    for (int i = 0; i < NUM_FS; ++i) {
        file_systems[i].mount = dentry_alloc(root_fs->mnt_mount, file_systems[i].target, T_IFDIR);

        if (!file_systems[i].mount) {
            kprint("vfs - failed to dentry for /%s\n", file_systems[i].target);
            continue;
        }

        if (vfs_mount_pseudo(file_systems[i].target, file_systems[i].type, file_systems[i].mount) < 0)
            kprint("vfs - failed to mount %s to /%s!\n", file_systems[i].type, file_systems[i].target);
    }

    // initialize character and block drivers
    //
    // these must be initialized after mounting the pseudo filesystems
    // (mainly devfs) because `/dev/char` and `/dev/block` directories
    // will be created during initialization
    if ((errno = cdev_init()) < 0) {
        kprint("vfs - error: %s\n", errno);
        kpanic("cdev_init() failed!");
    }

    fs_ctx_cache   = mm_cache_create(sizeof(fs_ctx_t));
    file_ctx_cache = mm_cache_create(sizeof(file_ctx_t));
}

int vfs_install_rootfs(char *type, void *data)
{
    kprint("vfs - initialize rootfs %s 0x%x\n", type, data);

    fs_type_t *fs = hm_get(fs_types, type);

    if (!fs)
        return -ENOTSUP;

    if (root_fs->mnt_sb)
        return -EBUSY;

    if (!(root_fs->mnt_sb = initramfs_get_sb(fs, NULL, 0, data)))
        return -errno;

    root_fs->mnt_root    = root_fs->mnt_sb->s_root;
    root_fs->mnt_type    = "initramfs";
    root_fs->mnt_devname = NULL;

    list_append(&superblocks, &root_fs->mnt_sb->s_list);

    return 0;
}

int vfs_register_fs(fs_type_t *fs)
{
    if (!fs || !fs->get_sb || !fs->kill_sb)
        return -EINVAL;

    if (hm_get(fs_types, (char *)fs->fs_name))
        return -EEXIST;

    return hm_insert(fs_types, (char *)fs->fs_name, fs);
}

int vfs_unregister_fs(char *type)
{
    if (!type || !hm_get(fs_types, type))
        return -EINVAL;

    FOREACH(mountpoints, m) {
        mount_t *mnt = container_of(m, mount_t, mnt_list);

        if (!kstrcmp_s(type, mnt->mnt_type))
            return -EBUSY;
    }

    return hm_remove(fs_types, type);
}

int vfs_mount(char *source, char *target,
              char *type,   uint32_t flags)
{
    (void)flags;

    fs_type_t *fs   = NULL;
    path_t    *path = NULL;
    mount_t   *mnt  = NULL;
    dentry_t  *src  = NULL,
              *dst  = NULL;

    if (!target || !type)
        return -EINVAL;

    if (!(fs = hm_get(fs_types, (char *)type))) {
        kprint("vfs - file system '%s' has not been registered!\n", type);
        return -ENOTSUP;
    }

    // devfs, sysfs and procfs can be mounted only once
    if (!kstrcmp_s(type, "devfs") || !kstrcmp_s(type, "sysfs") || !kstrcmp_s(type, "procfs")) {
        if (source) {
            kprint("vfs - %s doesn't take source!\n", type);
            return -EINVAL;
        }

        // go through the mounted filesystems and check if filesystem
        // of "type" has already been mounted -> return error
        FOREACH(mountpoints, m) {
            mnt = container_of(m, mount_t, mnt_list);

            if (!kstrcmp_s(mnt->mnt_type, type)) {
                kprint("vfs - %s has already been mounted to %s\n", type, target);
                return -EEXIST;
            }
        }

        goto check_dest;
    }

    // if source is NULL, the only possible (valid) filesystem is tmpfs
    if (!source) {
        if (kstrcmp_s(type, "tmpfs")) {
            kprint("vfs - source can't be NULL for %s\n", type);
            return -EINVAL;
        }
    } else {
        if (!(path = vfs_path_lookup(source, 0))->p_dentry) {
            kprint("vfs - '%s' does not exist!\n", source);
            return -ENOENT;
        }
    }

check_dest:
    // now check that "target" exist and it doesn't have a filesystem installed on it
    if (!(path = vfs_path_lookup(target, 0))->p_dentry) {
        kprint("vfs - mountpoint %s does not exist!\n", target);
        return -ENOENT;
    }

    // TODO: there must a better way to do this
    FOREACH(mountpoints, m) {
        mnt = container_of(m, mount_t, mnt_list);

        if (!kstrcmp_s(mnt->mnt_type, type)) {
            kprint("vfs - %s has already been mounted to %s\n", type, target);
            return -EEXIST;
        }
    }

    if (!(mnt = alloc_empty_mount())) {
        kprint("vfs - failed to allocate mountpoint for %s\n", type);
        return -ENOMEM;
    }

    mnt->mnt_sb      = fs->get_sb(fs, source, 0, NULL);
    mnt->mnt_type    = type;
    mnt->mnt_devname = source;
    mnt->mnt_mount   = dst;

    dst->d_count++;
    mnt->mnt_root = NULL; // TODO: how to get this from the filesystem

    list_append(&mountpoints, &mnt->mnt_list);

    return 0;
}

static char *vfs_extract_child(char **path)
{
    char *start = *path,
         *ptr   = *path;
    size_t len  = kstrlen(*path);
    size_t i    = 0;

    for (i = 0; i < len && ptr[i] != '/' && ptr[i] != '\0'; ++i)
        (*path)++;

    // there are more dentries left, add null byte to mark next start
    if (i != len) {
        (*path)++;
        ptr[i] = '\0';
    } else {
        // we're at the end set path to NULL so vfs_walk_path() knows to end the path lookup
        *path = NULL;
    }

    if (!len)
        return NULL;

    return start;
}

static dentry_t *vfs_walk_path(dentry_t *parent, char *path, int flags)
{
    if (!parent || !path) {
        errno = EINVAL;
        return NULL;
    }

    dentry_t *dntr = NULL;
    inode_t *ino   = NULL;
    char *next     = vfs_extract_child(&path);

    // we're at the end of path
    if (!path) {
        if (flags & LOOKUP_PARENT)
            return parent;

        // return dentry if found from parent's hashmap
        if ((dntr = hm_get(parent->d_children, next)))
            return dntr;

        if (!(ino = inode_lookup(parent, next)))
            return NULL;

        if (!(dntr = dentry_alloc_ino(parent, next, ino, ino->i_flags)))
            return NULL;

        return dntr;
    }

    if (!parent->d_children) {
        kprint("vfs - parent ('%s') doesn't have a valid children hashmap!\n", parent->d_name);
        errno = EINVAL;
        return NULL;
    }

    // check if parent has this dentry already cached, continue immediately if so
    if ((dntr = hm_get(parent->d_children, next)))
        return vfs_walk_path(dntr, path, flags);

    // not found from the parent hashmap, do filesystem-specific search
    if (!(ino = inode_lookup(parent, next)))
        return NULL;

    if (!(dntr = dentry_alloc_ino(parent, next, ino, ino->i_flags)))
        return NULL;

    return vfs_walk_path(dntr, path, flags);
}

static dentry_t *vfs_find_bootstrap(char **path)
{
    char *orig     = NULL,
         *tmp      = NULL,
         *mount    = NULL;
    dentry_t *dntr = NULL;

    // skip '/'
    kassert(**path == '/');
    (*path)++;

    // if the path is at the end already, it means that
    // user wanted the dentry of root
    if (**path == '\0') {
        *path = NULL;
        return root_fs->mnt_root;
    }

    orig  = tmp = kstrdup(*path);
    mount = vfs_extract_child(&tmp);

    FOREACH(mountpoints, m) {
        mount_t *mnt = container_of(m, mount_t, mnt_list);

        if (kstrcmp_s(mnt->mnt_mount->d_name, mount) == 0) {
            dntr     = mnt->mnt_root;
            (*path) += kstrlen(mount);

            if (**path == '\0')
                *path = NULL;
            else
                (*path)++;

            goto end;
        }
    }

end:
    kfree(orig);
    return dntr;
}

path_t *vfs_path_lookup(char *path, int flags)
{
    path_t *retpath   = kmalloc(sizeof(path_t));
    retpath->p_status = LOOKUP_STAT_ENOENT;
    retpath->p_flags  = flags;

    if (!path || !kstrlen(path)) {
        retpath->p_status = LOOKUP_STAT_EINVAL;
        retpath->p_dentry = NULL;
        return retpath;
    }

    dentry_t *start = NULL,
             *dntr  = NULL;
    char *_path_ptr = NULL,
         *_path     = NULL;

    _path_ptr = _path = kstrdup(path);

    if (_path[0] == '/') {
        start = vfs_find_bootstrap(&_path);

        if (_path == NULL) {
            if (flags & LOOKUP_PARENT)
                retpath->p_dentry = root_fs->mnt_root;
            else
                retpath->p_dentry = start;

            if ((flags & LOOKUP_CREATE) && (start != NULL))
                retpath->p_status = LOOKUP_STAT_EEXISTS;

            if ((flags & LOOKUP_OPEN) && (start != NULL))
                retpath->p_status = LOOKUP_STAT_SUCCESS;

            goto end;
        }

        // we got here so path is not NULL meaning there's more elements to process in the path
        // check if the bootstrap dentry is NULL.
        // if it is means that the first element after first '/' wasn't a mountpoint
        // and we must default to root_fs
        if (!start && !(start = root_fs->mnt_root)) {
            kpanic("rootfs missing");
        }
    } else {
        kpanic("relative paths not supported");
    }

    if (!start) {
        kprint("vfs - bootstrap dentry is NULL for %s!\n", _path_ptr);
        retpath->p_status = LOOKUP_STAT_EINVAL;
        errno = EINVAL;
        goto end;
    }

    if (!(retpath->p_dentry = vfs_walk_path(start, _path, flags))) {

        // intention was to open file but it doesn't exist -> path lookup "failed"
        if (flags & LOOKUP_OPEN)
            retpath->p_status = LOOKUP_STAT_ENOENT;

        if (flags & LOOKUP_CREATE) {
            retpath->p_status = LOOKUP_STAT_SUCCESS;

            if (flags & LOOKUP_PARENT)
                retpath->p_dentry = start;
            else
                retpath->p_dentry = NULL;
        }

        goto end;
    }

    // intention was to create file but it already exists -> path lookup "failed"
    if (flags & LOOKUP_CREATE && !(flags & LOOKUP_PARENT))
        retpath->p_status = LOOKUP_STAT_EEXISTS;

    // intention was to open file and it exists -> success
    if (flags & LOOKUP_OPEN)
        retpath->p_status = LOOKUP_STAT_SUCCESS;

    retpath->p_dentry->d_count++;

end:
    kfree(_path_ptr);
    return retpath;
}

int vfs_path_release(path_t *path)
{
    if (!path || !path->p_dentry)
        return -EINVAL;

    if (--path->p_dentry->d_count == 1)
        dentry_dealloc(path->p_dentry);

    kfree(path);

    return 0;
}

fs_ctx_t *vfs_alloc_fs_ctx(dentry_t *pwd)
{
    fs_ctx_t *ctx = mm_cache_alloc_entry(fs_ctx_cache);

    if (!ctx)
        return NULL;

    ctx->pwd   = pwd;
    ctx->root  = root_fs->mnt_root;
    ctx->count = 1;

    return ctx;
}

file_ctx_t *vfs_alloc_file_ctx(int numfd)
{
    file_ctx_t *ctx = mm_cache_alloc_entry(file_ctx_cache);

    if (!ctx)
        return NULL;

    ctx->count = 1;
    ctx->numfd = numfd;
    ctx->fd    = kmalloc(sizeof(file_t *) * numfd);

    return ctx;
}

int vfs_alloc_fd(file_ctx_t *ctx)
{
    if (!ctx)
        return -EINVAL;

    int ret  = ctx->numfd++;
    void *fd = kzalloc(sizeof(file_t *) * ctx->numfd);

    kmemcpy(fd, ctx->fd, sizeof(file_t *) * ctx->numfd - 1);
    kfree(ctx->fd);
    ctx->fd = fd; // TODO: atomic

    if (!(ctx->fd[ret] = file_generic_alloc()))
        return -ENOMEM;

    return ret;
}

int vfs_free_fs_ctx(fs_ctx_t *ctx)
{
    if (!ctx)
        return -EINVAL;

    if (ctx->count >= 2) {
        ctx->count--;
        return -EBUSY;
    }

    (void)dentry_dealloc(ctx->root);
    (void)dentry_dealloc(ctx->pwd);

    return 0;
}

int vfs_free_file_ctx(file_ctx_t *ctx)
{
    if (!ctx)
        return -EINVAL;

    if (ctx->count >= 2) {
        ctx->count--;
        return -EBUSY;
    }

    if (ctx->numfd > 0)
        kfree(ctx->fd);

    return 0;
}
