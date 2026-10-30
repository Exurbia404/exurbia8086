#include "console.h"

typedef unsigned short uint16_t;
typedef unsigned char uint8_t;

static const uint16_t width = 80;
static const uint16_t height = 25;
static uint16_t cursor;
static uint8_t color;

static inline void write_cell(uint16_t position, uint16_t value) {
    __asm__ volatile (
        "movw $0xB800, %%ax\n\t"
        "movw %%ax, %%es\n\t"
        "movw %0, %%es:(%%di)"
        :
        : "r"(value), "D"(position * 2)
        : "ax", "es", "memory"
    );
}

static inline uint16_t read_cell(uint16_t position) {
    uint16_t value;
    __asm__ volatile (
        "movw $0xB800, %%ax\n\t"
        "movw %%ax, %%es\n\t"
        "movw %%es:(%%di), %0"
        : "=r"(value)
        : "D"(position * 2)
        : "ax", "es", "memory"
    );
    return value;
}

static inline void write_port(uint16_t port, uint8_t value) {
    __asm__ volatile ("outb %0, %1" : : "a"(value), "Nd"(port));
}

static void update_cursor() {
    write_port(0x3D4, 0x0F);
    write_port(0x3D5, static_cast<uint8_t>(cursor));
    write_port(0x3D4, 0x0E);
    write_port(0x3D5, static_cast<uint8_t>(cursor >> 8));
}

static void scroll_if_needed() {
    if (cursor < width * height)
        return;

    for (uint16_t position = 0; position < width * (height - 1); position++)
        write_cell(position, read_cell(position + width));

    const uint16_t blank = (static_cast<uint16_t>(color) << 8) | ' ';
    for (uint16_t position = width * (height - 1); position < width * height; position++)
        write_cell(position, blank);

    cursor -= width;
}

void console_init() {
    cursor = 0;
    color = 0x0F;
    kclear();
}

void kclear() {
    const uint16_t blank = (static_cast<uint16_t>(color) << 8) | ' ';
    for (uint16_t position = 0; position < width * height; position++)
        write_cell(position, blank);
    cursor = 0;
    update_cursor();
}

void ksetcolor(unsigned char foreground, unsigned char background) {
    color = (background << 4) | (foreground & 0x0F);
}

void kputchar(char character) {
    if (character == '\n') {
        cursor = ((cursor / width) + 1) * width;
    } else if (character == '\r') {
        cursor -= cursor % width;
    } else if (character == '\b') {
        if (cursor > 0) {
            --cursor;
            write_cell(cursor, (static_cast<uint16_t>(color) << 8) | ' ');
        }
    } else if (character == '\t') {
        const uint16_t spaces = 8 - (cursor % width) % 8;
        for (uint16_t i = 0; i < spaces; ++i)
            kputchar(' ');
        return;
    } else {
        write_cell(cursor, (static_cast<uint16_t>(color) << 8) |
                           static_cast<unsigned char>(character));
        ++cursor;
    }

    scroll_if_needed();
    update_cursor();
}

void kputs(const char* string) {
    while (*string)
        kputchar(*string++);
}
