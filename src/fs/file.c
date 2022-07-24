#include <fs/file.h>
#include <fs/fs.h>
#include <kernel/kpanic.h>
#include <mm/slab.h>
#include <errno.h>

static mm_cache_t *file_cache     = NULL;
static mm_cache_t *file_ops_cache = NULL;

void file_init(void)
{
    if (!(file_cache = mm_cache_create(sizeof(file_t))))
        kpanic("failed to initialize slab cache for file objects!");

    if (!(file_ops_cache = mm_cache_create(sizeof(file_ops_t))))
        kpanic("failed to initialize slab cache for file ops!");
}

file_t *file_generic_alloc(void)
{
    file_t *file = NULL;

    if (!(file = mm_cache_alloc_entry(file_cache))) {
        errno = ENOMEM;
        return NULL;
    }

    if (!(file->f_ops = mm_cache_alloc_entry(file_ops_cache))) {
        mm_cache_free_entry(file_cache, file);
        errno = ENOMEM;
        return NULL;
    }

    file->f_count = 1;
    file->f_pos   = 0;

    return file;
}

int file_generic_dealloc(file_t *file)
{
    if (!file)
        return -EINVAL;

    if (file->f_count > 1)
        return -EBUSY;

    if (file->f_dentry)
        file->f_dentry->d_count--;

    mm_cache_free_entry(file_ops_cache, file->f_ops);
    mm_cache_free_entry(file_cache,     file);

    return 0;
}

int file_generic_seek(file_t *file, off_t off)
{
    if (!file || !file->f_dentry || !file->f_dentry->d_inode)
        return -EINVAL;

    const uint32_t FILE_SIZE = file->f_dentry->d_inode->i_size;

    if (file->f_pos < -off || file->f_pos + off > (off_t)FILE_SIZE)
        return -ESPIPE;

    file->f_pos = file->f_pos + off;
    return 0;
}

file_t *file_open(dentry_t *dntr, int mode)
{
    if (!dntr) {
        errno = EINVAL;
        return NULL;
    }

    if (!dntr->d_inode || !dntr->d_inode->i_fops) {
        errno = EINVAL;
        return NULL;
    }

    if (!dntr->d_inode->i_fops->open) {
        errno = ENOSYS;
        return NULL;
    }

    return dntr->d_inode->i_fops->open(dntr, mode);
}

void file_close(file_t *file)
{
    if (!file) {
        errno = EINVAL;
        return;
    }

    if (!file->f_ops->close) {
        errno = ENOSYS;
        return;
    }

    file->f_ops->close(file);
}

int file_seek(file_t *file, off_t off)
{
    if (!file)
        return -EINVAL;

    if (!file->f_ops->seek)
        return -ENOSYS;

    return file->f_ops->seek(file, off);
}

ssize_t file_read(file_t *file, off_t offset, size_t size, void *buffer)
{
    if (!file || !buffer)
        return -EINVAL;

    if (!file->f_ops->read)
        return -ENOSYS;

    return file->f_ops->read(file, offset, size, buffer);
}

ssize_t file_write(file_t *file, off_t offset, size_t size, void *buffer)
{
    if (!file || !buffer)
        return -EINVAL;

    if (!file->f_ops->write)
        return -ENOSYS;

    return file->f_ops->write(file, offset, size, buffer);
}
