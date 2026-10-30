#include "process.h"

typedef unsigned short uint16_t;

static const uint16_t max_processes = 3;
static const uint16_t process_stack_size = 0x0200;
static const uint16_t process_stack_base = 0x7000;
static const uint16_t initial_flags = 0x0202;

enum ProcessState {
    PROCESS_UNUSED,
    PROCESS_READY,
    PROCESS_RUNNING,
    PROCESS_EXITED
};

struct ProcessControlBlock {
    uint16_t process_id;
    uint16_t state;
    uint16_t stack_pointer;
    uint16_t entry;
    uint16_t exit_status;
};

static ProcessControlBlock processes[max_processes];
static ProcessControlBlock* current_process;
static uint16_t boot_stack_pointer;
static uint16_t next_process_id;
static bool initialized;
static bool started;

static void process_bootstrap();

static ProcessControlBlock* find_next_ready(uint16_t after_index) {
    for (uint16_t offset = 1; offset <= max_processes; ++offset) {
        uint16_t index = (after_index + offset) % max_processes;
        if (processes[index].state == PROCESS_READY)
            return &processes[index];
    }
    return 0;
}

static void initialize_stack(ProcessControlBlock* process, uint16_t index) {
    uint16_t stack_top = process_stack_base +
                         (index + 1) * process_stack_size;
    uint16_t* stack = reinterpret_cast<uint16_t*>(stack_top);

    *--stack = reinterpret_cast<uint16_t>(&process_bootstrap);
    *--stack = initial_flags;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    *--stack = 0;
    process->stack_pointer = reinterpret_cast<uint16_t>(stack);
}

static void process_bootstrap() {
    ProcessEntry entry =
        reinterpret_cast<ProcessEntry>(current_process->entry);
    entry();
    process_exit_current(0);
}

bool process_init() {
    if (initialized)
        return false;

    for (uint16_t i = 0; i < max_processes; ++i) {
        processes[i].process_id = 0;
        processes[i].state = PROCESS_UNUSED;
        processes[i].stack_pointer = 0;
        processes[i].entry = 0;
        processes[i].exit_status = 0;
    }
    current_process = 0;
    boot_stack_pointer = 0;
    next_process_id = 1;
    started = false;
    initialized = true;
    return true;
}

short process_create(ProcessEntry entry) {
    if (!initialized || started || !entry)
        return -1;

    for (uint16_t i = 0; i < max_processes; ++i) {
        ProcessControlBlock* process = &processes[i];
        if (process->state != PROCESS_UNUSED)
            continue;

        process->process_id = next_process_id++;
        process->state = PROCESS_READY;
        process->entry = reinterpret_cast<uint16_t>(entry);
        process->exit_status = 0;
        initialize_stack(process, i);
        return static_cast<short>(process->process_id);
    }
    return -1;
}

void process_start() {
    if (!initialized || started)
        return;

    ProcessControlBlock* first = find_next_ready(max_processes - 1);
    if (!first)
        return;

    started = true;
    current_process = first;
    first->state = PROCESS_RUNNING;
    process_context_switch(&boot_stack_pointer, first->stack_pointer);
    current_process = 0;
}

void process_yield() {
    if (!started || !current_process)
        return;

    uint16_t current_index =
        static_cast<uint16_t>(current_process - processes);
    ProcessControlBlock* next = find_next_ready(current_index);
    if (!next)
        return;

    ProcessControlBlock* previous = current_process;
    previous->state = PROCESS_READY;
    next->state = PROCESS_RUNNING;
    current_process = next;
    process_context_switch(&previous->stack_pointer, next->stack_pointer);
}

unsigned short process_current_id() {
    return current_process ? current_process->process_id : 0;
}

void process_exit_current(unsigned short status) {
    if (!current_process)
        for (;;)
            __asm__ volatile ("cli\n\thlt");

    ProcessControlBlock* previous = current_process;
    uint16_t current_index =
        static_cast<uint16_t>(previous - processes);
    previous->exit_status = status;
    previous->state = PROCESS_EXITED;

    ProcessControlBlock* next = find_next_ready(current_index);
    if (next) {
        next->state = PROCESS_RUNNING;
        current_process = next;
        process_context_switch(&previous->stack_pointer, next->stack_pointer);
    } else {
        current_process = 0;
        process_context_switch(&previous->stack_pointer, boot_stack_pointer);
    }

    for (;;)
        __asm__ volatile ("cli\n\thlt");
}
