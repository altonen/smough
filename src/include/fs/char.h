#ifndef __CHAR_DEV_H__
#define __CHAR_DEV_H__

#include <fs/fs.h>
#include <sys/types.h>

typedef struct cdev cdev_t;
file_ops_t *cdev_ops;

struct cdev {
    char *c_name;      // device name
    dev_t c_dev;       // device number
    file_ops_t *c_ops; // device options

    list_head_t c_list; // list of character devices
};

// initialize character driver subsystem
int cdev_init(void);

// allocate new character device
cdev_t *cdev_alloc(char *name, file_ops_t *ops, int major);

// deallocate character device
void cdev_dealloc(cdev_t *dev);

#endif /* __CHAR_DEV_H__ */
