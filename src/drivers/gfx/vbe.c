#include <arch/amd64/mmu.h>
#include <drivers/console/tty.h>
#include <drivers/gfx/vbe.h>
#include <drivers/bus/pci.h>
#include <mm/heap.h>
#include <mm/mmu.h>
#include <mm/page.h>
#include <mm/types.h>
#include <kernel/io.h>
#include <kernel/kassert.h>
#include <kernel/kprint.h>
#include <kernel/util.h>
#include <kernel/kpanic.h>
#include <stdbool.h>

#define DISPLAY_WIDTH     1024
#define DISPLAY_HEIGHT     768
#define DISPLAY_BITDEPTH     8

#define DISPLAY_FG_COLOR  0x22
#define DISPLAY_BG_COLOR  0xff

#define CURSOR_FG_COLOR  0xff
#define CURSOR_BG_COLOR  0x22

#define ROWS_PER_BANK       64
#define LAST_BANK           12

#define LINE_SIZE (DISPLAY_WIDTH * DISPLAY_BITDEPTH * 2)

static bool lfb          = false;
static uint8_t *vga_mem  = NULL;
static uint8_t *font_map = NULL;

static uint32_t cur_x    = 0;
static uint32_t cur_y    = 0;
static uint32_t cur_bank = 0;

static void *line_buffer[2] = {
    NULL, NULL
};

static inline void vbe_write_reg(uint16_t index, uint16_t value)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    outw(VBE_DISPI_IOPORT_DATA,  value);
}

static inline uint16_t vbe_read_reg(uint16_t index)
{
    outw(VBE_DISPI_IOPORT_INDEX, index);
    return inw(VBE_DISPI_IOPORT_DATA);
}

static inline void vbe_set_bank(uint16_t num)
{
    vbe_write_reg(VBE_DISPI_INDEX_BANK, num);
    cur_bank = num;
}

static inline void vbe_putpixel(uint8_t color, uint32_t y, uint32_t x)
{
    vga_mem[(y * DISPLAY_WIDTH + x - 1)] = color;
}

static inline void vbe_putchar(unsigned char c, uint32_t y, uint32_t x, int fgcolor, int bgcolor)
{
    static const int mask[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
    unsigned char *glyph = font_map + (int)c * 16;

    for (size_t cy = 0; cy < 16; ++cy) {
        for (size_t cx = 0; cx < 8; ++cx)
            vbe_putpixel(glyph[cy] & mask[cx] ? fgcolor : bgcolor, y + cy, x + 8 - cx);
    }
}

static void vbe_get_font(void)
{
    outw(0x3ce, 5);
    outw(0x3ce, 0x406);
    outw(0x3ce, 0x402);
    outw(0x3ce, 0x604);

    uint64_t page = mm_block_alloc(MM_ZONE_NORMAL, 1, 0);
    font_map      = (uint8_t *)amd64_p_to_v(page);
    vga_mem       = (uint8_t *)amd64_p_to_v(0xa0000);

	// copy font from vga memory to font map
    asm volatile("mov %[vga_ptr],  %%rsi\n\
                  mov %[font_ptr], %%rdi"
              :: [vga_ptr] "r" (vga_mem), [font_ptr] "r" (font_map));

    for (int i = 0; i < 256; ++i) {
        asm volatile ("movsd");
        asm volatile ("movsd");
        asm volatile ("movsd");
        asm volatile ("movsd");
        asm volatile ("add $16, %rsi");
    }

    // restore vga to normal state
    outw(0x3ce, 0x302);
    outw(0x3ce, 0x204);
    outw(0x3ce, 0x1005);
    outw(0x3ce, 0xe06);
}

void vbe_init(void)
{
    line_buffer[0] = amd64_p_to_v(mm_block_alloc(MM_ZONE_NORMAL, 2, 0));
    line_buffer[1] = amd64_p_to_v(mm_block_alloc(MM_ZONE_NORMAL, 2, 0));

    vbe_get_font();

    /* set video mode */
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_DISABLED);
    vbe_write_reg(VBE_DISPI_INDEX_XRES,   DISPLAY_WIDTH);
    vbe_write_reg(VBE_DISPI_INDEX_YRES,   DISPLAY_HEIGHT);
    vbe_write_reg(VBE_DISPI_INDEX_BPP,    DISPLAY_BITDEPTH);
    vbe_write_reg(VBE_DISPI_INDEX_ENABLE, VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED);

    /* install new tty print routines */
    tty_install_putc(vbe_put_char);
    tty_install_puts(vbe_put_str);

    pci_dev_t *dev = pci_get_dev(VBE_VENDOR_ID, VBE_DEVICE_ID);

    if (dev) {
        lfb     = true;
        vga_mem = (uint8_t *)((uint64_t)dev->bar0 - 8);

        for (uint64_t i = 0, ptr = (uint64_t)vga_mem; i < PAGE_SIZE; ++i, ptr += PAGE_SIZE)
            amd64_map_page(ptr, ptr, MM_PRESENT | MM_READWRITE);
    }

    vbe_clear_screen();
}

void vbe_clear_screen(void)
{
    if (!lfb) {
        kmemset(vga_mem, 0xff, 4096 * 4096);
    } else {
        for (size_t bank = 0; bank < LAST_BANK; ++bank) {
            vbe_set_bank(bank);
            kmemset(vga_mem, 0xff, PAGE_SIZE * 16);
        }
        vbe_set_bank(0);
    }

    cur_x = cur_y = 0;
}

void vbe_put_pixel(uint8_t color, uint32_t y, uint32_t x)
{
    vbe_putpixel(color, y, x);
}

static void __put_char(char c, bool cursor)
{
    kassert(vga_mem != NULL);

    if (cursor) {
        vbe_putchar(c, cur_y, cur_x, CURSOR_FG_COLOR, CURSOR_BG_COLOR);
        return;
    }

    switch (c) {
        case '\n':
            vbe_putchar(' ', cur_y, cur_x, DISPLAY_FG_COLOR, DISPLAY_BG_COLOR);
            cur_y += DISPLAY_BITDEPTH * 2;

            if (cur_bank == LAST_BANK)
                cur_y += DISPLAY_BITDEPTH * 6;
            cur_x = 0;
            break;

        case '\b':
            vbe_putchar(' ', cur_y, cur_x, DISPLAY_FG_COLOR, DISPLAY_BG_COLOR);
            vbe_putchar(0, cur_y, cur_x - DISPLAY_BITDEPTH, DISPLAY_FG_COLOR, DISPLAY_BG_COLOR);
            cur_x -= DISPLAY_BITDEPTH;
            break;

        case '\t':
            for (int i = 0; i < 4; ++i) {
                vbe_putchar(' ', cur_y, cur_x, DISPLAY_FG_COLOR, DISPLAY_BG_COLOR);
                cur_x += DISPLAY_BITDEPTH;
            }
            break;

        default:
            vbe_putchar(c, cur_y, cur_x, DISPLAY_FG_COLOR, DISPLAY_BG_COLOR);
            cur_x += DISPLAY_BITDEPTH;
            break;
    }

    /* end of line */
    if (cur_x >= DISPLAY_WIDTH) {
        cur_x = 0;
        cur_y += DISPLAY_BITDEPTH * 2;
    }

    if (lfb) {
        if ((cur_y / (DISPLAY_BITDEPTH * 2)) > 47) {
            for (size_t i = 0; i < 48; ++i)
                kmemcpy(vga_mem + i * LINE_SIZE, vga_mem + (i + 1) * LINE_SIZE, LINE_SIZE);

            if (c == '\n')
                cur_y = DISPLAY_BITDEPTH * 2 * 47;
        }
    } else {
        if (cur_y >= ROWS_PER_BANK) {
            if (cur_bank < LAST_BANK) {
                cur_x = cur_y = 0;
                vbe_set_bank(cur_bank + 1);
                goto end;
            }

            for (int bank = cur_bank; bank >= 0; --bank) {
                kmemcpy(line_buffer[0], vga_mem, LINE_SIZE);

                for (size_t i = 0; i < 3; ++i) {
                    kmemset(vga_mem + i * LINE_SIZE, 0x0, LINE_SIZE);
                    kmemcpy(vga_mem + i * LINE_SIZE, vga_mem + (i + 1) * LINE_SIZE, LINE_SIZE);
                }

                if (cur_bank != LAST_BANK)
                    kmemcpy(vga_mem + 3 * LINE_SIZE, line_buffer[1], LINE_SIZE);

                kmemcpy(line_buffer[1], line_buffer[0], LINE_SIZE);
                vbe_set_bank(cur_bank - 1);
            }

            cur_x = cur_y = 0;
            vbe_set_bank(LAST_BANK);
        }
    }

end:
    if (c == '\n')
        kmemset(vga_mem + cur_y * DISPLAY_WIDTH, 0x00, LINE_SIZE);
}

void vbe_put_char(char c)
{
    __put_char(c, false);

    if (c != '\n')
        __put_char(' ', true);
}

void vbe_put_str(char *s)
{
    kassert(s != NULL);

    size_t len = kstrlen(s);
    kassert(len != 0);

    for (size_t i = 0; i < len; ++i)
        __put_char(s[i], false);

    if (s[len - 1] != '\n')
        __put_char(' ', true);
}
