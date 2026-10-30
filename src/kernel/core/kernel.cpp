#include "../drivers/console/console.h"
#include "../drivers/keyboard/keyboard.h"
#include "../mm/heap.h"

static char line[80];

static void print_unsigned(unsigned short value) {
    char digits[5];
    unsigned short count = 0;
    do {
        digits[count++] = '0' + value % 10;
        value /= 10;
    } while (value && count < sizeof(digits));
    while (count)
        kputchar(digits[--count]);
}

#ifdef EXURBIA_QEMU
static bool memory_self_test() {
    MemoryStats initial;
    MemoryStats current;
    if (!memory_get_stats(&initial))
        return false;
    if (kmalloc(0) || kmalloc(initial.total_bytes + 2) ||
        kfree(0) || kfree(reinterpret_cast<void*>(0x2000)))
        return false;

    void* first = kmalloc(32);
    void* second = kmalloc(65);
    if (!first || !second || first == second)
        return false;

    static_cast<unsigned char*>(first)[0] = 0x5A;
    static_cast<unsigned char*>(second)[0] = 0xA5;
    if (static_cast<unsigned char*>(first)[0] != 0x5A ||
        static_cast<unsigned char*>(second)[0] != 0xA5)
        return false;

    if (!kfree(first))
        return false;
    void* reused = kmalloc(16);
    if (reused != first || !kfree(reused) || kfree(reused) || !kfree(second) ||
        !memory_get_stats(&current) ||
        current.free_bytes != initial.free_bytes ||
        current.allocation_count != 0)
        return false;

    void* largest = kmalloc(initial.total_bytes);
    if (!largest || !kfree(largest) ||
        !memory_get_stats(&current) ||
        current.free_bytes != initial.free_bytes)
        return false;

    return true;
}
#endif

static bool is_exit_command(const char* command, unsigned short length) {
    return length == 4 &&
           command[0] == 'e' &&
           command[1] == 'x' &&
           command[2] == 'i' &&
           command[3] == 't';
}

static void halt_system() {
    for (;;)
        __asm__ volatile ("cli\n\thlt");
}

static void exit_system() {
#ifdef EXURBIA_QEMU
    __asm__ volatile (
        "outb %0, %1"
        :
        : "a"(static_cast<unsigned char>(0x10)), "Nd"(0x00F4)
    );
#endif
    halt_system();
}

extern "C" __attribute__((section(".text.start"))) void _start() {
    console_init();
    if (!memory_init()) {
        kputs("Memory initialization failed.\n");
        halt_system();
    }

    ksetcolor(15, 0);
    kputs("exurbia8086 console\n");
    kputs("BIOS keyboard input, line editing enabled.\n");

    MemoryStats memory;
    if (!memory_get_stats(&memory)) {
        kputs("Heap metadata is invalid.\n");
        halt_system();
    }
    kputs("Heap: ");
    print_unsigned(memory.total_bytes);
    kputs(" bytes available.\n");

#ifdef EXURBIA_QEMU
    if (!memory_self_test()) {
        ksetcolor(12, 0);
        kputs("Memory allocator self-test failed.\n");
        halt_system();
    }
    ksetcolor(10, 0);
    kputs("Memory allocator self-test passed.\n");
#endif

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
        print_unsigned(length);
        kputs(" chars)");
        ksetcolor(15, 0);
    }
}
