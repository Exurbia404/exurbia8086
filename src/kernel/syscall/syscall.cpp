#include "syscall.h"

#include "../drivers/console/console.h"
#include "../drivers/keyboard/keyboard.h"
#include "../process/process.h"

typedef unsigned char uint8_t;
typedef unsigned short uint16_t;

static const uint16_t syscall_vector = 0x80;
static const uint16_t max_result = 0x7FFF;
static char input_line[80];
static uint16_t input_length;
static uint16_t input_offset;
static bool input_newline_pending;
static bool initialized;

static short invoke_syscall(
    uint16_t number,
    uint16_t argument1,
    uint16_t argument2,
    uint16_t argument3
) {
    uint16_t result;
    __asm__ volatile (
        "int $0x80"
        : "=a"(result)
        : "0"(number), "b"(argument1), "c"(argument2), "d"(argument3)
        : "cc", "memory"
    );
    return static_cast<short>(result);
}

bool syscall_init() {
    if (initialized)
        return false;

    uint16_t flags;
    uint16_t code_segment;
    __asm__ volatile (
        "pushf\n\t"
        "popw %0\n\t"
        "cli\n\t"
        "movw %%cs, %1"
        : "=r"(flags), "=r"(code_segment)
        :
        : "memory"
    );

    volatile uint16_t* vector =
        reinterpret_cast<volatile uint16_t*>(syscall_vector * 4);
    vector[0] = reinterpret_cast<uint16_t>(&syscall_interrupt_handler);
    vector[1] = code_segment;

    __asm__ volatile ("pushw %0\n\tpopf" : : "r"(flags) : "memory", "cc");
    input_length = 0;
    input_offset = 0;
    input_newline_pending = false;
    initialized = true;
    return true;
}

static short read_console(uint16_t file_descriptor, uint16_t buffer, uint16_t count) {
    if (file_descriptor != 0)
        return SYS_ERROR_BAD_FILE_DESCRIPTOR;
    if (count > max_result || (count && (!buffer || count > 0xFFFF - buffer)))
        return SYS_ERROR_INVALID_ARGUMENT;
    if (!count)
        return 0;

    if (input_offset == input_length && !input_newline_pending) {
        input_length = kgetline(input_line, sizeof(input_line));
        input_offset = 0;
        input_newline_pending = true;
    }

    volatile uint8_t* output = reinterpret_cast<volatile uint8_t*>(buffer);
    uint16_t bytes_read = 0;
    while (bytes_read < count && input_offset < input_length)
        output[bytes_read++] = static_cast<uint8_t>(input_line[input_offset++]);

    if (bytes_read < count && input_offset == input_length && input_newline_pending) {
        output[bytes_read++] = '\n';
        input_newline_pending = false;
    }

    return static_cast<short>(bytes_read);
}

static short write_console(uint16_t file_descriptor, uint16_t buffer, uint16_t count) {
    if (file_descriptor != 1 && file_descriptor != 2)
        return SYS_ERROR_BAD_FILE_DESCRIPTOR;
    if (count > max_result || (count && (!buffer || count > 0xFFFF - buffer)))
        return SYS_ERROR_INVALID_ARGUMENT;

    const volatile uint8_t* input = reinterpret_cast<const volatile uint8_t*>(buffer);
    for (uint16_t i = 0; i < count; ++i)
        kputchar(static_cast<char>(input[i]));
    return static_cast<short>(count);
}

extern "C" short syscall_dispatch_frame(SystemCallFrame* frame) {
    if (!frame)
        return SYS_ERROR_INVALID_ARGUMENT;

    short result;
    switch (frame->ax) {
    case SYS_READ:
        result = read_console(frame->bx, frame->cx, frame->dx);
        break;
    case SYS_WRITE:
        result = write_console(frame->bx, frame->cx, frame->dx);
        break;
    case SYS_OPEN:
    case SYS_CLOSE:
    case SYS_FORK:
        result = SYS_ERROR_NOT_IMPLEMENTED;
        break;
    case SYS_YIELD:
        process_yield();
        result = 0;
        break;
    case SYS_EXIT:
        if (process_current_id())
            process_exit_current(frame->bx);
#ifdef EXURBIA_QEMU
        __asm__ volatile (
            "outb %0, %1"
            :
            : "a"(static_cast<uint8_t>(0x10)), "Nd"(0x00F4)
        );
#endif
        for (;;)
            __asm__ volatile ("cli\n\thlt");
    default:
        result = SYS_ERROR_NOT_IMPLEMENTED;
        break;
    }

    return result;
}

short syscall_read(
    unsigned short file_descriptor,
    unsigned short buffer_offset,
    unsigned short count
) {
    return invoke_syscall(SYS_READ, file_descriptor, buffer_offset, count);
}

short syscall_write(
    unsigned short file_descriptor,
    unsigned short buffer_offset,
    unsigned short count
) {
    return invoke_syscall(SYS_WRITE, file_descriptor, buffer_offset, count);
}

short syscall_open(unsigned short path_offset, unsigned short flags) {
    return invoke_syscall(SYS_OPEN, path_offset, flags, 0);
}

short syscall_close(unsigned short file_descriptor) {
    return invoke_syscall(SYS_CLOSE, file_descriptor, 0, 0);
}

short syscall_fork() {
    return invoke_syscall(SYS_FORK, 0, 0, 0);
}

short syscall_yield() {
    return invoke_syscall(SYS_YIELD, 0, 0, 0);
}

void syscall_exit(unsigned short status) {
    invoke_syscall(SYS_EXIT, status, 0, 0);
    for (;;)
        __asm__ volatile ("cli\n\thlt");
}
