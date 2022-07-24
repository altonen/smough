#ifndef __INITRAMFS_H__
#define __INITRAMFS_H__

#include <fs/fs.h>

superblock_t *initramfs_get_sb(fs_type_t *type, char *dev, int flags, void *data);
int initramfs_kill_sb(superblock_t *sb);

#endif /* __INITRAMFS_H__ */
