#include <fs/dentry.h>
#include <fs/devfs.h>
#include <fs/super.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <errno.h>

static list_head_t chr_devs;
static list_head_t blk_devs;

static inode_t *devfs_inode_lookup(dentry_t *parent, char *name);
static int devfs_mkdir(dentry_t *dir, dentry_t *dntr, int mode);
static int devfs_rmdir(dentry_t *dir, dentry_t *dntr);
static int devfs_mknod(dentry_t *dir, dentry_t *dntr, int mode, dev_t rdev);

static int devfs_mkdir(dentry_t *dir, dentry_t *dntr, int mode)
{
    (void)mode;

    if (!dir || !dntr)
        return -EINVAL;

    if (!(dir->d_flags & T_IFDIR) || !(dntr->d_flags & T_IFDIR))
        return -EINVAL;

    if (hm_get(dir->d_children, dntr->d_name))
        return -EEXIST;

    if ((errno = -hm_insert(dir->d_children, dntr->d_name, dntr)) > 0)
        return -errno;

    list_append(&dir->d_list, &dntr->d_list);

    return 0;
}

static int devfs_rmdir(dentry_t *dir, dentry_t *dntr)
{
    if (!dir || !dntr)
        return -EINVAL;

    if (dntr->d_count > 1)
        return -EBUSY;

    return dentry_dealloc(dntr);
}

static int devfs_mknod(dentry_t *dir, dentry_t *dntr, int mode, dev_t rdev)
{
    (void)mode, (void)rdev;

    if (!dir || !dntr)
        return -EINVAL;

    return -ENOSYS;
}

static inode_t *devfs_inode_lookup(dentry_t *parent, char *name)
{
    (void)parent, (void)name;
    return NULL;
}

static inode_t *devfs_inode_alloc(superblock_t *sb)
{
    if (!sb) {
        errno = EINVAL;
        return NULL;
    }

    inode_t *ino = NULL;

    if (!(ino = inode_generic_alloc(0)))
        return NULL;

    ino->i_uid  = 0;
    ino->i_gid  = 0;
    ino->i_size = 0;
    ino->i_sb   = sb;

    ino->i_iops->lookup = devfs_inode_lookup;
    ino->i_iops->mknod  = devfs_mknod;
    ino->i_iops->mkdir  = devfs_mkdir;
    ino->i_iops->rmdir  = devfs_rmdir;

    ino->i_fops->read     = NULL; ino->i_fops->open        = NULL;
    ino->i_fops->close    = NULL; ino->i_fops->seek        = NULL;
    ino->i_iops->create   = NULL; ino->i_iops->link        = NULL;
    ino->i_iops->symlink  = NULL; ino->i_iops->unlink      = NULL;
    ino->i_iops->rename   = NULL; ino->i_iops->follow_link = NULL;
    ino->i_iops->put_link = NULL; ino->i_iops->truncate    = NULL;
    ino->i_fops->write    = NULL; ino->i_iops->permission  = NULL;

    list_append(&ino->i_sb->s_ino, &ino->i_list);

    return ino;
}

static int devfs_inode_destroy(inode_t *ino)
{
    (void)ino;
    return -ENOSYS;
}

static int devfs_inode_read(inode_t *ino)
{
    (void)ino;
    return -ENOSYS;
}

static int devfs_fill_super(superblock_t *sb)
{
    if (!sb)
        return -EINVAL;

    sb->s_blocksize = 0;
    sb->s_count     = 1;
    sb->s_dirty     = 0;
    sb->s_magic     = 0;
    sb->s_private   = NULL;

    sb->s_root          = dentry_alloc_orphan("dev", T_IFDIR);
    sb->s_root->d_inode = devfs_inode_alloc(sb);

    sb->s_ops->destroy_inode = devfs_inode_destroy;
    sb->s_ops->alloc_inode   = devfs_inode_alloc;
    sb->s_ops->read_inode    = devfs_inode_read;

    sb->s_ops->delete_inode = NULL;
    sb->s_ops->write_super  = NULL;
    sb->s_ops->dirty_inode  = NULL;
    sb->s_ops->write_inode  = NULL;
    sb->s_ops->put_inode    = NULL;
    sb->s_ops->put_super    = NULL;
    sb->s_ops->sync_fs      = NULL;

    list_init(&sb->s_ino);
    list_init(&sb->s_ino_dirty);
    list_init(&sb->s_instances);
    list_init(&chr_devs);
    list_init(&blk_devs);

    return 0;
}

superblock_t *devfs_get_sb(fs_type_t *type, char *dev, int flags, void *data)
{
    return super_get_sb_single(type, dev, flags, data, devfs_fill_super);
}

int devfs_kill_sb(superblock_t *sb)
{
    if (!sb)
        return -EINVAL;

    if (sb->s_count > 1)
        return -EBUSY;

    return 0;
}

int devfs_register_cdev(cdev_t *dev, char *name)
{
    int ret         = 0;
    char *path      = kstrcat_s("/dev/", name);
    path_t *retpath = vfs_path_lookup(path, LOOKUP_PARENT | LOOKUP_CREATE);
    dentry_t *dntr  = NULL;
    inode_t *ino    = NULL;

    if (retpath->p_status & LOOKUP_STAT_EEXISTS) {
        kdebug("devfs - %s already exists!", path);
        ret = -EEXIST;
        goto end;
    }

    if (!(ino = devfs_inode_alloc(retpath->p_dentry->d_inode->i_sb))) {
        ret = -errno;
        goto end;
    }

    if (!(dntr = dentry_alloc_ino(retpath->p_dentry, name, ino, T_IFCHR))) {
        (void)devfs_inode_destroy(ino);
        ret = -errno;
        goto end;
    }

    ino->i_cdev  = dev;
    ino->i_flags = T_IFCHR;
    ino->i_fops  = dev->c_ops;

    list_append(&chr_devs, &dev->c_list);

end:
    vfs_path_release(retpath);
    return ret;
}

int devfs_unregister_cdev(cdev_t *dev)
{
    (void)dev;

    return 0;
}

int devfs_register_bdev(bdev_t *dev, char *name)
{
    // TODO: make symbolic link to /dev/block/dev->major:dev->minor
    (void)dev, (void)name;

    return 0;
}

int devfs_unregister_bdev(bdev_t *dev)
{
    (void)dev;

    return 0;
}

int devfs_alloc_minor(int major)
{
    (void)major;

    // TODO: zzz
    static int next_minor = 1;
    return next_minor++;
}
