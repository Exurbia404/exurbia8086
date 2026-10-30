#include "console.h"
#include "keyboard.h"

static char line[80];

static bool is_exit_command(const char* command, unsigned short length) {
    return length == 4 &&
           command[0] == 'e' &&
           command[1] == 'x' &&
           command[2] == 'i' &&
           command[3] == 't';
}

static void exit_system() {
#ifdef EXURBIA_QEMU
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(static_cast<unsigned char>(0x10)), "Nd"(0x00F4)
    );
#endif

    for (;;)
        __asm__ volatile ("cli\n\thlt");
}

extern "C" void _start() {
    console_init();
    ksetcolor(15, 0);
    kputs("exurbia8086 console\n");
    kputs("BIOS keyboard input, line editing enabled.\n");

    while (1) {
        ksetcolor(11, 0);
        kputs("\n> ");
        ksetcolor(15, 0);
        unsigned short length = kgetline(line, sizeof(line));
        if (is_exit_command(line, length)) {
            ksetcolor(10, 0);
            kputs("Exiting.\n");
            exit_system();
        }

        ksetcolor(10, 0);
        kputs("You typed: ");
        kputs(line);
        ksetcolor(8, 0);
        kputs(" (");
        char digits[5];
        unsigned short value = length;
        unsigned short count = 0;
        do {
            digits[count++] = '0' + value % 10;
            value /= 10;
        } while (value && count < sizeof(digits));
        while (count)
            kputchar(digits[--count]);
        kputs(" chars)");
        ksetcolor(15, 0);
    }
}
