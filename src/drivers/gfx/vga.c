#include <kernel/util.h>
#include <stddef.h>
#include <stdint.h>

enum vga_color {
	VGA_COLOR_BLACK = 0,
	VGA_COLOR_BLUE = 1,
	VGA_COLOR_GREEN = 2,
	VGA_COLOR_CYAN = 3,
	VGA_COLOR_RED = 4,
	VGA_COLOR_MAGENTA = 5,
	VGA_COLOR_BROWN = 6,
	VGA_COLOR_LIGHT_GREY = 7,
	VGA_COLOR_DARK_GREY = 8,
	VGA_COLOR_LIGHT_BLUE = 9,
	VGA_COLOR_LIGHT_GREEN = 10,
	VGA_COLOR_LIGHT_CYAN = 11,
	VGA_COLOR_LIGHT_RED = 12,
	VGA_COLOR_LIGHT_MAGENTA = 13,
	VGA_COLOR_LIGHT_BROWN = 14,
	VGA_COLOR_WHITE = 15,
};

static const size_t VGA_WIDTH = 80;
static const size_t VGA_HEIGHT = 25;
static volatile uint16_t *vga_buffer = (uint16_t *)0xFFFFFFFF800B8000;

static uint8_t term_color;
static size_t term_row, term_col;

static inline void tty_entry(char c, uint8_t color, size_t x, size_t y)
{
	size_t index = y * VGA_WIDTH + x;
	vga_buffer[index] = c | (color << 8);
}

void vga_init(void)
{
    term_row = term_col = 0;
	term_color = VGA_COLOR_LIGHT_GREEN | VGA_COLOR_BLACK << 4;

	for (size_t x = 0; x < VGA_WIDTH; x++) {
		for (size_t y = 0; y < VGA_HEIGHT; y++) {
			tty_entry(0x0, term_color, x, y);
		}
	}
}

void vga_put_char(char c)
{
	switch (c) {
		case '\n':
			term_row++;
			term_col = 0;
			break;

		case '\b':
			tty_entry(0x0, term_color, term_col, term_row);
			term_col--;
			break;

		case '\t': {
			for (int i = 0; i < 4; ++i)
				tty_entry(' ', term_color, term_col++, term_row);
			break;
		}

		default: {
			tty_entry(c, term_color, term_col, term_row);
			term_col++;
			break;
		}
	}

	if (term_col >= VGA_WIDTH) {
		term_col = 0;
		term_row++;
	}

	if (term_row >= VGA_HEIGHT) {
		for (size_t row = 1; row < VGA_HEIGHT; ++row) {
			for (size_t col = 0; col < VGA_WIDTH; ++col) {
				int prev = (VGA_WIDTH * row) + col - VGA_WIDTH;
				int cur  = (VGA_WIDTH * row) + col;
				vga_buffer[prev] = vga_buffer[cur];
			}
		}

		term_row = VGA_HEIGHT - 1;
		if (c == '\n') { /* clear last line */
			for (size_t col = 0; col < VGA_WIDTH; ++col) {
				size_t index = (VGA_WIDTH * term_row) + col;
				vga_buffer[index] = (0x0 | term_color << 8);
			}
		}
	}
}

void vga_put_str(char *data)
{
	size_t size = kstrlen(data);
	for (size_t i = 0; i < size; ++i)
		vga_put_char(data[i]);
}

