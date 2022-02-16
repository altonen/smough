#ifndef __TYPES_H__
#define __TYPES_H__

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __amd64__
#undef uint64_t
typedef unsigned long     uint64_t;
typedef unsigned long int uint64_t;
typedef unsigned int      uint32_t;

typedef long int int64_t;
typedef int      int32_t;

typedef uint64_t size_t;
typedef int64_t  ssize_t;
typedef int64_t  off_t;

typedef size_t uintptr_t;

#undef  ULONG_MAX
#define ULONG_MAX 0xffffffffffffffff
#else
typedef unsigned long int uint32_t;
typedef unsigned long     uint32_t;
typedef unsigned int      uint32_t;

typedef long int int32_t;
typedef int      int32_t;

typedef uint32_t size_t;
typedef int32_t  ssize_t;
typedef int32_t  off_t;

#undef  ULONG_MAX
#define ULONG_MAX 0xffffffff
#endif

typedef int gid_t;
typedef uint32_t dev_t;
typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

#endif /* __TYPES_H__ */
