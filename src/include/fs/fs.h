#ifndef __VFS_H__
#define __VFS_H__

#include <fs/file.h>
#include <fs/super.h>
#include <lib/list.h>

enum FILE_MODES {
    T_IFMT  = 1 << 0,
    T_IFBLK = 1 << 1,
    T_IFCHR = 1 << 2,
    T_IFIFO = 1 << 3,
    T_IFREG = 1 << 4,
    T_IFDIR = 1 << 5,
    T_IFLNK = 1 << 6
};

enum FILE_ACCESS_MODES {
    O_EXEC   = 1 << 0,
    O_RDONLY = 1 << 1,
    O_RDWR   = 1 << 2,
    O_WRONLY = 1 << 3,
    O_APPEND = 1 << 4,
    O_CREAT  = 1 << 5,
    O_TRUNC  = 1 << 6
};

enum LOOKUP_FLAGS {
    LOOKUP_PARENT = 1 << 0,
    LOOKUP_CREATE = 1 << 1,
    LOOKUP_OPEN   = 1 << 2,
};

enum LOOKUP_STATUS_FLAGS {
    LOOKUP_STAT_SUCCESS       = 0 << 0, // successful path lookup
    LOOKUP_STAT_ENOENT        = 1 << 0, // intention was to open file
    LOOKUP_STAT_EEXISTS       = 1 << 1, // intention was to create file
    LOOKUP_STAT_EINVAL        = 1 << 2, // invalid path given
    LOOKUP_STAT_INV_BOOTSTRAP = 1 << 3, // bootstrap dentry is invalid
};

typedef struct fs_type {
    char *fs_name;

    superblock_t *(*get_sb)(struct fs_type *, char *, int, void *);
    int (*kill_sb)(superblock_t *);
} fs_type_t;

typedef struct mount {
    superblock_t *mnt_sb; // superblock of the mounted filesystem
    dentry_t *mnt_mount;  // dentry of the mountpoint
    dentry_t *mnt_root;   // dentry of the root dir of the mounted filesystem

    char *mnt_devname;    // device name of the filesystem (e.g. /dev/sda1)
    char *mnt_type;       // type of the mounted filesystem

    list_head_t mnt_list; // list of mountpoints
} mount_t;

typedef struct path {
    dentry_t *p_dentry; // set to NULL if nothing was found
    int p_flags;        // copy of the flags used for path lookup
    int p_status;       // status of the path lookup (see LOOKUP_STATUS_FLAGS)
} path_t;

// - initialize emptry rootfs
// - register all known filesystem
// - initialize char and block device drivers
// - initialize binfmt
// - create dentries for /, /sys, /tmp, /dev and /proc
// - mount pseudo filesystems
//   - devfs  to /dev
//   - tmpfs  to /tmp
//   - procfs to /proc
//   - sysfs  to /sys
void vfs_init(void);

// register filesystem
//
// only registered filesystems can be mounted
int vfs_register_fs(fs_type_t *fs);

// unregister filesystem
int vfs_unregister_fs(char *type);

// mount filesystem of type "type" to "target" from "source" using "flags"
int vfs_mount(char *source, char *target, char *type, uint32_t flags);

// install new root file system of type `type`
//
// this function just installs filesystem "initramfs" to current
// root (root_fs). After that user can execute programs from, e.g., /sbin/init
int vfs_install_rootfs(char *type, void *data);

// do a path lookup
//
// NOTE: returned path must be explicitly freed by calling `vfs_path_release()`
//
// `vfs_path_lookup()` always returns a path_t object but on error or if the path
// did not resolve to anything (e.g. nothing was found) nthe path_t->p_dentry
// is NULL and errno is set.
//
// If something was indeed found, path->p_dentry points to the object in question */
path_t *vfs_path_lookup(char *path, int flags);

// allocate file system context
//
// toot dentry is set automatically, "pwd" may be NULL
fs_ctx_t *vfs_alloc_fs_ctx(dentry_t *pwd);

// allocate file context object for "numfd" file system descriptors
//
// this routine allocates pointers for the descriptors but not
// the actual memory for the them
file_ctx_t *vfs_alloc_file_ctx(int numfd);

// allocate space for a new file object in "ctx" and
//
// return the the file descriptor (index) of the new file.
int vfs_alloc_fd(file_ctx_t *ctx);

// free the file system context pointed to by "ctx"
int vfs_free_fs_ctx(fs_ctx_t *ctx);

// free the file context pointed to by "ctx"
int vfs_free_file_ctx(file_ctx_t *ctx);

// release path
int vfs_path_release(path_t *path);

#endif /* end of include guard: __VFS_H__ */
