#ifndef __BIMFMT_H__
#define __BIMFMT_H__

#include <fs/file.h>
#include <lib/list.h>
#include <stdbool.h>

typedef bool (*binfmt_loader_t)(file_t *, int, char **);

void binfmt_init(void);
void binfmt_add_loader(binfmt_loader_t loader);
bool binfmt_load(file_t *file, int argc, char **argv);

#endif /* __BIMFMT_H__ */
