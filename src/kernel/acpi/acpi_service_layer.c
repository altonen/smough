#include <drivers/bus/pci.h>
#include <kernel/acpi/acpica/acpi.h>
#include <kernel/irq.h>
// #include <sync/spinlock.h>
#include <kernel/io.h>
#include <kernel/kprint.h>
#include <mm/heap.h>
#include <mm/mmu.h>

extern unsigned long acpi_get_rspd(void);

ACPI_STATUS AcpiOsInitialize()
{
    return AE_OK;
}

void AcpiOsPrintf(const char *fmt, ...)
{
    va_list args;
    va_start(args, fmt);
    vkprint(fmt, args);
    va_end(args);
}

void *AcpiOsAllocate(ACPI_SIZE size)
{
    return kmalloc(size);
}

void AcpiOsFree(void *ptr)
{
    kfree(ptr);
}

void *AcpiOsMapMemory(ACPI_PHYSICAL_ADDRESS addr, ACPI_SIZE length)
{
    (void)length;

    /* kdebug("0x%x", addr); */
    /* if ((addr & 0xff) == 0xca) { */
    /*     /1* for (;;); *1/ */
    /* } */

    return (void *)addr;
}

void AcpiOsUnmapMemory(void *addr, ACPI_SIZE length)
{
    (void)addr, (void)length;
}

ACPI_STATUS AcpiOsWriteMemory(ACPI_PHYSICAL_ADDRESS Address, uint64_t Value, uint32_t width)
{
    kdebug("NOT IMPLEMENTED");
    return AE_ERROR;
}

BOOLEAN AcpiOsReadable(void *memory, ACPI_SIZE length)
{
    kdebug("NOT IMPLEMENTED");
    return AE_ERROR;
}

ACPI_STATUS AcpiOsReadMemory(ACPI_PHYSICAL_ADDRESS Address, uint64_t *value, uint32_t width)
{
    kdebug("NOT IMPLEMENTED");
    return AE_ERROR;
}

void AcpiOsStall(uint32_t us)
{
    kdebug("NOT IMPLEMENTED");
}

void AcpiOsSleep(uint64_t ms)
{
    kdebug("NOT IMPLEMENTED");
}

void AcpiOsWaitEventsComplete (void)
{
    kdebug("NOT IMPLEMENTED");
}

ACPI_STATUS AcpiOsReadPciConfiguration(ACPI_PCI_ID *pci_id, UINT32 reg, UINT64 *value, UINT32 width)
{
    switch (width) {
        case 8:
            *value = pci_read_u8(pci_id->Bus, pci_id->Device, pci_id->Function, reg & ~3);
            return AE_OK;

        case 16:
            *value = pci_read_u16(pci_id->Bus, pci_id->Device, pci_id->Function, reg & ~3);
            return AE_OK;

        case 32:
            *value = pci_read_u32(pci_id->Bus, pci_id->Device, pci_id->Function, reg & ~3);
            return AE_OK;
    }
}

ACPI_STATUS AcpiOsWritePciConfiguration (ACPI_PCI_ID *pci_id, uint32_t Register, uint64_t Value,uint32_t Width)
{
    kdebug("NOT IMPLEMENTED");
    return AE_OK;
}

ACPI_STATUS AcpiOsCreateSemaphore(uint32_t max, uint32_t init, ACPI_SEMAPHORE *out_handle)
{
    *out_handle = (void *)0x1337;
    return AE_OK;
}

ACPI_STATUS AcpiOsRemoveInterruptHandler(uint32_t InterruptNumber, ACPI_OSD_HANDLER Handler)
{
    kdebug("NOT IMPLEMENTED");
    return AE_ERROR;
}

ACPI_STATUS AcpiOsInstallInterruptHandler(uint32_t InterruptLevel, ACPI_OSD_HANDLER Handler, void *Context)
{
    irq_install_handler(InterruptLevel, Handler, Context);
    return AE_OK;
}

ACPI_STATUS AcpiOsEnterSleep(uint8_t SleepState, uint32_t RegaValue, uint32_t RegbValue)
{
    kdebug("NOT IMPLEMENTED");
    return AE_ERROR;
}

ACPI_STATUS AcpiOsDeleteSemaphore(ACPI_SEMAPHORE handle)
{
    kfree(handle);
    return AE_OK;
}

/* From the specification: single-threaded implementations should always return AE_OK */
ACPI_STATUS AcpiOsWaitSemaphore(ACPI_SEMAPHORE handle, uint32_t units, uint16_t timeout)
{
    return AE_OK;
}

/* From the specification: single-threaded implementations should always return AE_OK */
ACPI_STATUS AcpiOsSignalSemaphore(ACPI_SEMAPHORE handle, uint32_t units)
{
    return AE_OK;
}

ACPI_STATUS AcpiOsCreateLock(ACPI_SPINLOCK *out_handle)
{
    *out_handle = (void *)0x1337;
    return AE_OK;
}

void AcpiOsDeleteLock(ACPI_HANDLE handle)
{
    kdebug("NOT IMPLEMENTED");
}

ACPI_CPU_FLAGS AcpiOsAcquireLock(ACPI_SPINLOCK handle)
{
    kdebug("NOT IMPLEMENTED");
}

void AcpiOsReleaseLock(ACPI_SPINLOCK handle, ACPI_CPU_FLAGS flags)
{
    kdebug("NOT IMPLEMENTED");
}

UINT64 AcpiOsGetTimer()
{
    /* kdebug("NOT IMPLEMENTED"); */
    return 0;
}

ACPI_STATUS AcpiOsSignal(uint32_t func, void *info)
{
    kdebug("NOT IMPLEMENTED");
    return AE_ERROR;
}

ACPI_STATUS AcpiOsWritePort(ACPI_IO_ADDRESS addr, uint32_t value, uint32_t width)
{
    switch (width) {
        case 32:
            outl(addr, value);
            break;

        case 16:
            outw(addr, value & 0xffff);
            break;

        case 8:
            outb(addr, value & 0xff);
            break;

        default:
            return AE_ERROR;
    }

    return AE_OK;
}

ACPI_STATUS AcpiOsReadPort(ACPI_IO_ADDRESS addr, uint32_t *value, uint32_t width)
{
    if (!value)
        return AE_ERROR;

    switch (width) {
        case 32:
            *value = inl(addr);
            break;

        case 16:
            *value = inw(addr);
            break;

        case 8:
            *value = inb(addr);
            break;

        default:
            return AE_ERROR;
    }

    return AE_OK;
}

ACPI_THREAD_ID AcpiOsGetThreadId(void)
{
    return 1;
}

ACPI_STATUS AcpiOsExecute(ACPI_EXECUTE_TYPE type, ACPI_OSD_EXEC_CALLBACK func, void *ctx)
{
    kdebug("NOT IMPLEMENTED");
    return AE_ERROR;
}

ACPI_STATUS AcpiOsTerminate()
{
    kdebug("NOT IMPLEMENTED");
    return AE_ERROR;
}

void AcpiOsVprintf(const char *fmt, va_list args)
{
    kdebug("NOT IMPLEMENTED");
}

ACPI_STATUS AcpiOsPredefinedOverride(const ACPI_PREDEFINED_NAMES *InitVal, ACPI_STRING *NewVal)
{
    return AE_NOT_CONFIGURED;
}

ACPI_STATUS AcpiOsTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_TABLE_HEADER **NewTable)
{
    return AE_NOT_CONFIGURED;
}

ACPI_STATUS AcpiOsPhysicalTableOverride(ACPI_TABLE_HEADER *ExistingTable, ACPI_PHYSICAL_ADDRESS *NewAddress, UINT32 *NewTableLength)
{
    return AE_NOT_CONFIGURED;
}

ACPI_PHYSICAL_ADDRESS AcpiOsGetRootPointer(void)
{
    ACPI_PHYSICAL_ADDRESS addr;

	AcpiFindRootPointer(&addr);
	return addr;
}
