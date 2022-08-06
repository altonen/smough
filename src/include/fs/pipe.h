#ifndef __PIPE_H__
#define __PIPE_H__

#include <sys/types.h>

typedef struct pipe pipe_t;

// init pipe slab caches
int pipe_init(void);

// create pipe
pipe_t *pipe_create(size_t size);

// destroy pipe
void pipe_destroy(pipe_t *pipe);

// open pipe
int pipe_open(pipe_t *pipe, int mode);

// close pipe
int pipe_close(pipe_t *pipe);

// read `size` bytes from `pipe` to `buffer`
//
// return the number of bytes read on success
// return `-EINVAL` if one of the arguments is invalid
ssize_t pipe_read(pipe_t *pipe, void *buffer, size_t size);

// write `size` bytes from `buffer` to `pipe`
//
// return the number of bytes written written on success (`size`)
// return `-EINVAL` if one the parameters is invalid
// return `-ENOMEM` if "size" is larger than pipe's capacity
// return `-ENOSPC` if there is not enough space in the pipe for `size` bytes
ssize_t pipe_write(pipe_t *pipe, void *buffer, size_t size);

#endif /* __PIPE_H__ */
