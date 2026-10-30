#ifndef KERNEL_PROCESS_H
#define KERNEL_PROCESS_H

typedef void (*ProcessEntry)();

bool process_init();
short process_create(ProcessEntry entry);
void process_start();
void process_yield();
unsigned short process_current_id();
void process_exit_current(unsigned short status) __attribute__((noreturn));

extern "C" void process_context_switch(
    unsigned short* outgoing_stack_pointer,
    unsigned short incoming_stack_pointer
);

#endif
