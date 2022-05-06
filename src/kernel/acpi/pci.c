#include <drivers/bus/pci.h>
#include <kernel/acpi/acpica/acpi.h>
#include <kernel/kassert.h>
#include <kernel/acpi/acpi.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <sys/types.h>
#include <errno.h>

static ACPI_STATUS __get_pci_current_res(ACPI_RESOURCE *res, void *ctx)
{
    pci_link_t *link = (pci_link_t *)ctx;

    switch (res->Type) {
        case ACPI_RESOURCE_TYPE_IRQ:
        {
            ACPI_RESOURCE_IRQ *irq = &res->Data.Irq;

            if (irq->InterruptCount == 0)
                return AE_OK;

            link->current = irq->Interrupts[0];
        }
        break;

        case ACPI_RESOURCE_TYPE_EXTENDED_IRQ:
        {
            ACPI_RESOURCE_EXTENDED_IRQ *irq = &res->Data.ExtendedIrq;

            if (irq->InterruptCount == 0)
                return AE_OK;

            link->current = irq->Interrupts[0];
        }
        break;

        case ACPI_RESOURCE_TYPE_END_TAG:
            break;

        default:
            kprint("pci - unknown current resource found, type %d", res->Type);
            break;
    }

    return AE_OK;
}

static ACPI_STATUS __get_pci_bus_info(ACPI_HANDLE BusDevice, uint32_t nest_lvl, void *ctx, void **res)
{
    (void)nest_lvl, (void)ctx, (void)res;

    ACPI_STATUS ret;

    size_t off     = 0;
    uint8_t bus    = 0;
    uint8_t device = 0;

    pci_irq_route_t *route            = NULL;
    ACPI_DEVICE_INFO *dev_info        = NULL;
    ACPI_PCI_ROUTING_TABLE *pci_route = NULL;

    char name[256];
    ACPI_BUFFER bus_name = {
        .Length  = sizeof(name),
        .Pointer = name
    };

    ACPI_OBJECT bus_number_obj;
    ACPI_BUFFER bus_number = {
        .Length = sizeof(bus_number_obj),
        .Pointer = &bus_number_obj
    };

    ACPI_PCI_ROUTING_TABLE IrqRoutingTable[256];
    ACPI_BUFFER irq_routing_table = {
        .Length = sizeof(IrqRoutingTable),
        .Pointer = IrqRoutingTable
    };

    if (ACPI_FAILURE(ret = AcpiGetName(BusDevice, ACPI_FULL_PATHNAME, &bus_name)))
        return ret;

    if (ACPI_FAILURE(ret = AcpiGetObjectInfo(BusDevice, &dev_info)))
        return ret;

    if (ACPI_FAILURE(ret = AcpiEvaluateObjectTyped(BusDevice, "_BBN", NULL, &bus_number, ACPI_TYPE_INTEGER))) {
        if (ret != AE_NOT_FOUND)
            return ret;
    } else {
        bus = bus_number_obj.Integer.Value;
    }

    if (ACPI_FAILURE(ret = AcpiGetIrqRoutingTable(BusDevice, &irq_routing_table)))
        return ret;

    /* Loop through the PCI IRQ Routing table and find routing information */
    while (off < irq_routing_table.Length) {
        pci_route = (ACPI_PCI_ROUTING_TABLE *)((uint8_t *)IrqRoutingTable + off);

        if (!pci_route->Length)
            break;

        device = (pci_route->Address >> 16) & 0xFF;
        route  = pci_alloc_route();

        route->bus = bus;
        route->dev = device;
        route->pin = pci_route->Pin;

        if (pci_route->SourceIndex == 0) {
            kstrcpy(route->name, pci_route->Source);
            route->named = true;
        } else {
            route->irq   = pci_route->SourceIndex;
            route->named = false;
        }

        off += pci_route->Length;
    }

    return AE_OK;
}

static ACPI_STATUS __get_pci_link_info(ACPI_HANDLE link_dev, uint32_t nest_lvl, void *ctx, void **res)
{
    (void)link_dev, (void)nest_lvl, (void)ctx, (void)res;

    ACPI_STATUS ret;

    char resource_buf[512];
    ACPI_BUFFER resbuf = {
        .Length  = sizeof(resource_buf),
        .Pointer = resource_buf
    };

    char link_name[256];
    ACPI_BUFFER namebuf = {
        .Length  = sizeof(link_name),
        .Pointer = link_name
    };

    if (ACPI_FAILURE(ret = AcpiGetName(link_dev, ACPI_FULL_PATHNAME, &namebuf)))
        return ret;

    pci_link_t *link = pci_alloc_link();

    link->current = 0;
    kmemcpy(link->name, namebuf.Pointer, namebuf.Length);

    /* PCI Links consist of two types of information:
     *    - current resources
     *    - possible resources
     *
     * Current resource is the one we're interested in for now at least */
    if (ACPI_FAILURE(ret = AcpiGetCurrentResources(link_dev, &resbuf)))
        return ret;

    if (ACPI_FAILURE(ret = AcpiWalkResourceBuffer(&resbuf, __get_pci_current_res, link)))
        return ret;

    return AE_OK;
}

int acpi_init_pci(void)
{
    ACPI_STATUS ret;

    /* Find information about the PCI links */
    if (ACPI_FAILURE(ret = AcpiGetDevices("PNP0C0F", __get_pci_link_info, NULL, NULL))) {
        kprint("ACPI INIT failure %s\n", AcpiFormatException(ret));
        return -ENXIO;
    }

    /* Probe PCI bus to find the PCI IRQ routing table */
    if (ACPI_FAILURE(ret = AcpiGetDevices("PNP0A03", __get_pci_bus_info, NULL, NULL))) {
        kprint("ACPI INIT failure %s\n", AcpiFormatException(ret));
        return -ENXIO;
    }

    return 0;
}
