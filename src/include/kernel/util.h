#ifndef __STR_UTIL_H__
#define __STR_UTIL_H__

#include <stddef.h>

void *kmemcpy(void *restrict dstptr, const void *restrict srcptr, size_t size);
void *kmemset(void *buf, int c, size_t size);
void *kmemmove(void *dstptr, const void *srcptr, size_t size);
void *kmemdup(void *src, size_t size);

char *kstrdup(const char *s);
char *kstrchr(const char *s, int c);
char *kstrsep(char **str, const char *delim);
char *kstrncpy(char *restrict dst, const char *restrict src, size_t n);
char *kstrcpy(char *dst, char *src);
char *kstrncat(char *s1, char *s2, size_t len);
char *kstrcat_s(char *s1, char *s2);

int kstrcmp(const char *s1, const char *s2);
int kstrcmp_s(const char *s1, const char *s2);
int kstrncmp(const char *s1, const char *s2, size_t len);
int kmemcmp(void *s1, void *s2, size_t n);
size_t kstrlen(const char *str);

#endif /* __STR_UTIL_H__ */
