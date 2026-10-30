#ifndef KERNEL_SYSCALL_H
#define KERNEL_SYSCALL_H

enum SystemCallNumber {
    SYS_READ = 0,
    SYS_WRITE = 1,
    SYS_OPEN = 2,
    SYS_CLOSE = 3,
    SYS_EXIT = 4,
    SYS_FORK = 5
};

enum SystemCallError {
    SYS_ERROR_NOT_IMPLEMENTED = -1,
    SYS_ERROR_BAD_FILE_DESCRIPTOR = -2,
    SYS_ERROR_INVALID_ARGUMENT = -3
};

struct SystemCallFrame {
    unsigned short es;
    unsigned short ds;
    unsigned short bp;
    unsigned short di;
    unsigned short si;
    unsigned short dx;
    unsigned short cx;
    unsigned short bx;
    unsigned short ax;
};

bool syscall_init();
short syscall_read(
    unsigned short file_descriptor,
    unsigned short buffer_offset,
    unsigned short count
);
short syscall_write(
    unsigned short file_descriptor,
    unsigned short buffer_offset,
    unsigned short count
);
short syscall_open(unsigned short path_offset, unsigned short flags);
short syscall_close(unsigned short file_descriptor);
short syscall_fork();
void syscall_exit(unsigned short status);

extern "C" short syscall_dispatch_frame(SystemCallFrame* frame);
extern "C" void syscall_interrupt_handler();

#endif
