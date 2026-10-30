#include "../drivers/console/console.h"
#include "../mm/heap.h"
#include "../syscall/syscall.h"

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

static void fail(const char* message) {
    ksetcolor(12, 0);
    kputs(message);
    halt_system();
}

static void syscall_self_test() {
    static const char message[] = "System call interface ready.\n";
    char input;

    if (syscall_write(
            1, reinterpret_cast<unsigned short>(message), sizeof(message) - 1
        ) != sizeof(message) - 1)
        fail("System call write failed.\n");
    if (syscall_write(
            3, reinterpret_cast<unsigned short>(message), 1
        ) != SYS_ERROR_BAD_FILE_DESCRIPTOR)
        fail("System call descriptor validation failed.\n");
    if (syscall_read(
            1, reinterpret_cast<unsigned short>(&input), 1
        ) != SYS_ERROR_BAD_FILE_DESCRIPTOR)
        fail("System call read validation failed.\n");
    if (syscall_read(0, 0, 1) != SYS_ERROR_INVALID_ARGUMENT ||
        syscall_write(1, 0, 1) != SYS_ERROR_INVALID_ARGUMENT ||
        syscall_write(1, 0xFFFE, 2) !=
            SYS_ERROR_INVALID_ARGUMENT)
        fail("System call argument validation failed.\n");
    if (syscall_open(reinterpret_cast<unsigned short>(""), 0) !=
            SYS_ERROR_NOT_IMPLEMENTED ||
        syscall_close(0) != SYS_ERROR_NOT_IMPLEMENTED ||
        syscall_fork() != SYS_ERROR_NOT_IMPLEMENTED)
        fail("Unsupported system call handling failed.\n");

    ksetcolor(10, 0);
    kputs("System call self-test passed.\n");
    ksetcolor(15, 0);
}

extern "C" __attribute__((section(".text.start"))) void _start() {
    console_init();
    if (!memory_init()) {
        kputs("Memory initialization failed.\n");
        halt_system();
    }
    if (!syscall_init()) {
        kputs("System call initialization failed.\n");
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
    syscall_self_test();

    while (1) {
        ksetcolor(11, 0);
        short prompt_result = syscall_write(
            1, reinterpret_cast<unsigned short>("\n> "), 3
        );
        if (prompt_result != 3) {
            if (prompt_result == SYS_ERROR_BAD_FILE_DESCRIPTOR)
                fail("System call prompt has a bad descriptor.\n");
            if (prompt_result == SYS_ERROR_INVALID_ARGUMENT)
                fail("System call prompt has invalid arguments.\n");
            fail("System call prompt returned an unexpected result.\n");
        }
        ksetcolor(15, 0);
        short bytes_read = syscall_read(
            0, reinterpret_cast<unsigned short>(line), sizeof(line)
        );
        if (bytes_read < 0)
            fail("System call read failed.\n");
        unsigned short length = static_cast<unsigned short>(bytes_read);
        if (length && line[length - 1] == '\n')
            --length;
        line[length] = '\0';

        if (is_exit_command(line, length)) {
            if (syscall_write(
                    1, reinterpret_cast<unsigned short>("Exiting.\n"), 9
                ) != 9)
                fail("System call exit message write failed.\n");
            syscall_exit(0);
        }

        ksetcolor(10, 0);
        short prefix_result = syscall_write(
            1, reinterpret_cast<unsigned short>("You typed: "), 11
        );
        short line_result = syscall_write(
            1, reinterpret_cast<unsigned short>(line), length
        );
        if (prefix_result != 11 || line_result != static_cast<short>(length))
            fail("System call response write failed.\n");
        ksetcolor(8, 0);
        kputs(" (");
        print_unsigned(length);
        kputs(" chars)");
        short newline_result = syscall_write(
            1, reinterpret_cast<unsigned short>("\n"), 1
        );
        if (newline_result != 1) {
            if (newline_result == SYS_ERROR_BAD_FILE_DESCRIPTOR)
                fail("System call newline has a bad descriptor.\n");
            if (newline_result == SYS_ERROR_INVALID_ARGUMENT)
                fail("System call newline has invalid arguments.\n");
            fail("System call newline returned an unexpected result.\n");
        }
        ksetcolor(15, 0);
    }
}
