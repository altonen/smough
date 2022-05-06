#include <drivers/device.h>
#include <kernel/compiler.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <mm/slab.h>
#include <errno.h>

static mm_cache_t *dev_cache    = NULL;
static mm_cache_t *driver_cache = NULL;

static struct {
    list_head_t pci;
} drivers;

static struct {
    list_head_t pci;
} devices;

int dev_init(void)
{
    if ((dev_cache = mm_cache_create(sizeof(device_t))))
        kprint("dev - failed to create SLAB cache for devices!\n");

    if (!(driver_cache = mm_cache_create(sizeof(driver_t))))
        kprint("dev - failed to create SLAB cache for drivers!\n");

    list_init(&drivers.pci);
    list_init(&devices.pci);

    return 0;
}

device_t *dev_alloc_device(void)
{
    device_t *dev = mm_cache_alloc_entry(dev_cache);

    if (!dev) {
        errno = ENOMEM;
        return NULL;
    }

    list_init(&dev->list);
    return dev;
}

driver_t *dev_alloc_driver(void)
{
    driver_t *driver = mm_cache_alloc_entry(driver_cache);

    if (!driver) {
        errno = ENOMEM;
        return NULL;
    }

    list_init(&driver->list);
    return driver;
}

int dev_register_device(device_t *device)
{
    if (!device)
        return -EINVAL;

    if (device->driver && device->driver->init)
        device->driver->init(device);

    list_append(&devices.pci, &device->list);

    return 0;
}

int dev_destroy_device(device_t *device)
{
    if (!device)
        return -EINVAL;

    list_remove(&device->list);
    return mm_cache_free_entry(dev_cache, device);
}

int dev_register_pci_driver(int vendor, int device, driver_t *driver)
{
    if (!driver)
        return -EINVAL;

    FOREACH(drivers.pci, iter) {
        driver_t *tmp = container_of(iter, driver_t, list);

        kassert(tmp != NULL);

        if (tmp->vendor == vendor && tmp->device == device) {
            kprint("dev - driver already exist for device 0x%x\n", device);
            return -EEXIST;
        }
    }

    list_append(&drivers.pci, &driver->list);

    return 0;
}

driver_t *dev_find_pci_driver(int vendor, int device)
{
    FOREACH(drivers.pci, iter) {
        driver_t *driver = container_of(iter, driver_t, list);

        kassert(driver != NULL);

        if (driver->vendor == vendor && driver->device == device)
            return driver;
    }

    errno = ENOENT;
    return NULL;
}
