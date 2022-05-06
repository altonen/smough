#ifndef __DEVICE_H__
#define __DEVICE_H__

#include <lib/list.h>
#include <sys/types.h>

enum DEVICE_TYPES {
    DT_PCI = 0
};

typedef struct driver {
    int (*init)(void *);
    int (*destroy)(void *);

    int count;
    int vendor;
    int device;
    list_head_t list;
} driver_t;

typedef struct device {
    driver_t *driver;
    int type;
    void *ctx;
    char *name;

    list_head_t list;
} device_t;

int dev_init(void);

device_t *dev_alloc_device(void);
driver_t *dev_alloc_driver(void);
int dev_register_device(device_t *device);
int dev_destroy_device(device_t *device);
int dev_register_pci_driver(int vendor, int device, driver_t *driver);
driver_t *dev_find_pci_driver(int vendor, int device);

#endif /* __DEVICE_H__ */
