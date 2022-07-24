#ifndef __DEVFS_H__
#define __DEVFS_H__

#include <fs/char.h>

#define DEV_MAJOR(dev)         ((dev >> 20) & 0xfff)
#define DEV_MINOR(dev)         (dev & 0xfffff)
#define MAKE_DEV(major, minor) (((major & 0xfff) << 20) | (minor & 0xfffff))

superblock_t *devfs_get_sb(fs_type_t *type, char *dev, int flags, void *data);
int devfs_kill_sb(superblock_t *sb);

int devfs_register_cdev(cdev_t *dev, char *name);
int devfs_unregister_cdev(cdev_t *dev);

int devfs_register_bdev(bdev_t *dev, char *name);
int devfs_unregister_bdev(bdev_t *dev);

int devfs_alloc_minor(int major);

#endif /* __DEVFS_H__ */
