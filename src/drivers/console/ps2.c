#include <drivers/ioapic.h>
#include <drivers/lapic.h>
#include <fs/file.h>
#include <fs/char.h>
#include <fs/fs.h>
#include <fs/pipe.h>
#include <fs/devfs.h>
#include <kernel/io.h>
#include <kernel/irq.h>
#include <mm/heap.h>
#include <errno.h>

#define PS2_PIPE_SIZE 64

enum {
    CONTROL  = 29,
    L_SHIFT  = 42,
    R_SHIFT  = 54,
    ALT      = 56,
    CAPSLOCK = 58,
    F1       = 59,
    F2       = 60,
    F3       = 61,
    F4       = 62,
    F5       = 63,
    F6       = 64,
    F7       = 65,
    F8       = 66,
    F9       = 67,
    F10      = 68,
    UP       = 72,
    LEFT     = 75,
    RIGHT    = 77,
    DOWN     = 80
};

unsigned char codes[256] = {
    [2]   = '1',     [3]   = '2',      [4]   = '3',     [5]  = '4',   [6]  = '5',
    [7]   = '6',     [8]   = '7',      [9]   = '8',     [10] = '9',   [11] = '0',
    [12]  = '-',     [13]  = '=',      [14]  = '\b',    [15] = '\t',  [16] = 'q',
    [17]  = 'w',     [18]  = 'e',      [19]  = 'r',     [20] = 't',   [21] = 'y',
    [22]  = 'u',     [23]  = 'i',      [24]  = 'o',     [25] = 'p',   [26] = '[',
    [27]  = ']',     [28]  = '\n',     [29]  = CONTROL, [30] = 'a',   [31] = 's',
    [32]  = 'd',     [33]  = 'f',      [34]  = 'g',     [35] = 'h',   [36] = 'j',
    [37]  = 'k',     [38]  = 'l',      [39]  = ';',     [40] = '\'',  [41] = '`',
    [42]  = L_SHIFT, [43]  = '\\',     [44]  = 'z',     [45] = 'x',   [46] = 'c',
    [47]  = 'v',     [48]  = 'b',      [49]  = 'n',     [50] = 'm',   [51] = ',',
    [52]  = '.',     [53]  = '/',      [54]  = R_SHIFT, [55] = '*',   [56] = ALT,
    [57]  = ' ',     [58]  = CAPSLOCK, [59]  = F1,      [60] = F2,    [61] = F3,
    [62]  = F4,      [63]  = F5,       [64]  = F6,      [65] = F7,    [66] = F8,
    [67]  = F9,      [68]  = F10,      [72]  = UP,      [75] = LEFT,  [77] = RIGHT,
    [78]  = '+',     [80]  = DOWN,

    /* key values when shift is down */
    [132] = '!',     [133] = '"',      [134] = '#',    [136] = '%',  [137] = '&',
    [138] = '/',     [139] = '(',      [140] = ')',    [141] = '=',  [142] = '_',
    [146] = 'Q',     [147] = 'W',      [148] = 'E',    [149] = 'R',  [150] = 'T',
    [151] = 'Y',     [152] = 'U',      [153] = 'I',    [154] = 'O',  [155] = 'P',
    [156] = '<',     [157] = '>',      [160] = 'A',    [161] = 'S',  [162] = 'D',
    [163] = 'F',     [164] = 'G',      [165] = 'H',    [166] = 'J',  [167] = 'K',
    [168] = 'L',     [174] = 'Z',      [175] = 'X',    [176] = 'C',  [177] = 'V',
    [178] = 'B',     [179] = 'N',      [180] = 'M',    [181] = ';',  [182] = ':',

};

static pipe_t *ps2_pipe = NULL;

static uint32_t __kbd_handler(void *ctx)
{
    (void)ctx;

    static int shift_down = 0;

    uint8_t sc = inb(0x60);

    if (sc & 0x80) {
        sc &= ~0x80;

        if (sc == R_SHIFT || sc == L_SHIFT)
            shift_down = 0;
    } else {
        if (sc == R_SHIFT || sc == L_SHIFT)
            shift_down = 1;
        else
            pipe_write(ps2_pipe, &codes[shift_down ? sc + 130 : sc], 1);
    }

    lapic_ack_interrupt();

    return IRQ_HANDLED;
}

static ssize_t __read(file_t *file, off_t offset, size_t size, void *buf)
{
    (void)offset;

    if (!file || !buf || !size)
        return -EINVAL;

    return pipe_read(file->f_private, buf, size);
}

static file_t *__open(dentry_t *dntr, int mode)
{
    kprint("ps2 - open /dev/bkd, mode bits %d\n", mode);

    if (mode != O_RDONLY) {
        errno = EINVAL;
        return NULL;
    }

    file_t *file = file_generic_alloc();

    if (!file)
        return NULL;

    file->f_ops     = dntr->d_inode->i_fops;
    file->f_mode    = mode;
    file->f_private = ps2_pipe;

    dntr->d_inode->i_count++;

    return file;
}

static int __close(file_t *file)
{
    return file_generic_dealloc(file);
}

int ps2_init(void)
{
    kprint("ps2 - initializing ps2 driver\n");

    file_ops_t *ops = NULL;
    cdev_t *dev     = NULL;
    int ret         = 0;

    if (!(ps2_pipe = pipe_create(PS2_PIPE_SIZE)))
        return -errno;

    if (!(ops = kmalloc(sizeof(file_ops_t))))
        goto error;

    ops->read  = __read;
    ops->open  = __open;
    ops->close = __close;
    ops->write = NULL;
    ops->seek  = NULL;

    if (!(dev = cdev_alloc("kbd", ops, 0)))
        goto error_ops;

    if ((ret = devfs_register_cdev(dev, "kbd")) < 0)
        goto error_cdev;

    // open the pipe for writing, all ttys and other
    // devices using `/dev/kbd` must open it with mode `O_RDONLY`
    if (pipe_open(ps2_pipe, O_WRONLY) < 0)
        goto error_cdev_unregister;

    kprint("ps2 - keyboard mapped to /dev/kbd, IRQ number %u\n", VECNUM_KEYBOARD);

    ioapic_enable_irq(0, VECNUM_KEYBOARD);
    irq_install_handler(VECNUM_KEYBOARD, __kbd_handler, NULL);

    return 0;

error_cdev_unregister:
    (void)devfs_unregister_cdev(dev);

error_cdev:
    (void)cdev_dealloc(dev);

error_ops:
    kfree(ops);

error:
    kfree(ps2_pipe);

	kprint("here");
    return -errno;
}
