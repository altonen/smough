#ifndef __ERRNO_H__
#define __ERRNO_H__

int errno;

#define ENOSPC   1      /* No space left on device */
#define ENOENT   2      /* No such file or directory */
#define EINVAL   3      /* Invalid value */
#define EEXIST   4      /* File exists */
#define ENOMEM   5      /* Out of memory */
#define EBUSY    6      /* Device busy */
#define E2BIG    7      /* Argument list too long */
#define ESPIPE   8      /* Illegal seek */
#define ENOSYS   9      /* Funcion not implemented */
#define ENOTDIR 10      /* Not a directory */
#define ENOTSUP 11      /* Operation not supported */
#define ENXIO   12      /* No such device or address */
#define EAGAIN  13      /* Try again */
#define EFAULT  14      /* Bad address */
#define EINPROGRESS 115 /* Operation now in progress */

#define EMAX    13 /* Used by the kstrerror() */

#endif /* __ERRNO_H__ */
