#include <fs/dentry.h>
#include <fs/fs.h>
#include <fs/file.h>
#include <fs/pipe.h>
#include <kernel/common.h>
#include <kernel/compiler.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <mm/heap.h>
#include <mm/slab.h>
#include <errno.h>

struct pipe {
    file_t *file;    // underlying file object of the pipe

    void *mem;       // buffer holding the pipe data
    size_t size;     // size of the pipe in bytes
    size_t ptr;      // read/write ptr of the pipe

    size_t nreaders; // number of readers on this pipe
    size_t nwriters; // number of writers on this pipe
};

static mm_cache_t *p_cache = NULL;

static ssize_t __read(file_t *file, off_t offset, size_t size, void *buffer)
{
    (void)offset;

    if (!file || !file->f_private || !buffer || !size)
        return -EINVAL;

    pipe_t *pipe = file->f_private;
    size_t nread = 0;

	if (!READ_ONCE(pipe->ptr))
        return 0;

    nread = (size < pipe->ptr) ? size : pipe->ptr;
    kmemcpy(buffer, pipe->mem, nread);

    if (size >= pipe->ptr) {
        pipe->ptr = 0;
    } else {
        kmemmove(pipe->mem, (uint8_t *)pipe->mem + size, pipe->ptr - nread);
        pipe->ptr = size + pipe->ptr - nread;
    }

    return nread;
}

static ssize_t __write(file_t *file, off_t offset, size_t size, void *buffer)
{
    (void)offset;

    if (!file || !file->f_private || !buffer || !size)
        return -EINVAL;

    pipe_t *pipe = file->f_private;

    if (pipe->size - pipe->ptr < size)
        return -ENOSPC;

    kmemcpy((uint8_t *)pipe->mem + pipe->ptr, buffer, size);
    pipe->ptr += size;

    return size;
}

static file_t *__open(dentry_t *dntr, int mode)
{
    if (!dntr || !dntr->d_private) {
        errno = EINVAL;
        return NULL;
    }

    pipe_t *pipe = dntr->d_private;

    if (mode == O_WRONLY)
        pipe->nwriters++;
    else if (mode == O_RDONLY)
        pipe->nreaders++;
    else
        kprint("pipe - invalid mode given for pipe_open()\n");

    return NULL;
}

static int __close(file_t *file)
{
    if (!file)
        return -EINVAL;

    pipe_t *pipe = file->f_private;

    if (file->f_mode == O_WRONLY)
        pipe->nwriters--;
    else if (file->f_mode == O_RDONLY)
        pipe->nreaders--;
    else
        kprint("pipe - invalid mode given for pipe_close()\n");

    return 0;
}

int pipe_init(void)
{
    if (!(p_cache = mm_cache_create(sizeof(pipe_t)))) {
        kprint("pipe - failed to allocate SLAB cache for pipe\n");
        return -ENOMEM;
    }

    return 0;
}

pipe_t *pipe_create(size_t size)
{
    if (!p_cache || !size) {
        errno = EINVAL;
        return NULL;
    }

    pipe_t *pipe = NULL;

    if (!(pipe = mm_cache_alloc_entry(p_cache)))
        return NULL;

    if (!(pipe->file = file_generic_alloc()))
        return NULL;

    pipe->file->f_ops->open  = __open;
    pipe->file->f_ops->close = __close;
    pipe->file->f_ops->read  = __read;
    pipe->file->f_ops->write = __write;

    /* save the pipe object to both file and dentry private data */
    pipe->file->f_private           = pipe;
    pipe->file->f_dentry->d_private = pipe;

    pipe->mem  = kmalloc(size);
    pipe->size = size;
    pipe->ptr  = 0;

    return pipe;
}

int pipe_open(pipe_t *pipe, int mode)
{
    if (!pipe)
        return -EINVAL;

    pipe->file->f_mode = mode;
    (void)pipe->file->f_ops->open(pipe->file->f_dentry, mode);

    return 0;
}

int pipe_close(pipe_t *pipe)
{
    if (!pipe)
        return -EINVAL;

    return pipe->file->f_ops->close(pipe->file);
}

ssize_t pipe_read(pipe_t *pipe, void *buffer, size_t size)
{
    if (!pipe || !buffer || !size)
        return -EINVAL;

    return pipe->file->f_ops->read(pipe->file, 0, size, buffer);
}

ssize_t pipe_write(pipe_t *pipe, void *buffer, size_t size)
{
    if (!pipe || !buffer || !size)
        return -EINVAL;

    return pipe->file->f_ops->write(pipe->file, 0, size, buffer);
}
