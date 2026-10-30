# Minimal process model

The kernel has a small cooperative scheduler for up to three kernel tasks.
Tasks run in real mode, share one address space and kernel privilege, and
switch only when they call `process_yield()` or the `SYS_YIELD` system call.
There is no timer-driven preemption, memory protection, or separate user mode.

## Context switching

Each process control block stores a process ID, state, entry point, exit status,
and saved stack pointer. The assembly context switch saves general registers
and flags on the outgoing task's stack, records its stack pointer, then restores
the selected task's stack and registers. Tasks share segment zero; the segment
registers are saved and restored but are not used to isolate memory.

Each process has a fixed 512-byte stack. The current scheduler supports at most
three process slots, reserved in physical memory from `0x7000` through
`0x75FF`. The original boot stack remains at `0x7C00`.

## Process API

Declared in `src/kernel/process/process.h`:

| Function | Behavior |
| --- | --- |
| `process_init()` | Initialize the fixed process table. |
| `process_create(entry)` | Create a ready task and return its positive process ID, or `-1` if invalid or full. Creation is available only before scheduling starts. |
| `process_start()` | Switch from the boot stack to the first ready task. Returns to the boot code after every task has exited. |
| `process_yield()` | Cooperatively give the CPU to the next ready task. Returns immediately when there is no other ready task. |
| `process_current_id()` | Return the running task's ID, or zero when executing on the boot stack. |
| `process_exit_current(status)` | Mark the running task exited and schedule another task; does not return. |

`SYS_YIELD` exposes cooperative yielding through the system-call interface.
`SYS_EXIT` ends the calling process. Once all tasks have exited, control returns
to the boot context, which shuts the emulator down in QEMU builds or halts the
CPU otherwise.

## Demonstration and limitations

The QEMU startup creates two short-lived workers and the interactive prompt as
three separate tasks. The workers print a message and yield; the prompt also
yields after handling each submitted line. This demonstrates that saved stacks
resume independently while all tasks share kernel data and console state.

The BIOS-backed keyboard line read blocks without yielding. While the prompt is
waiting for input, no other cooperative task can run. There is no idle task,
timer interrupt, task wait/reap operation, stack-overflow guard, or process
slot reuse. The reserved `fork` system call remains unimplemented; cloning a
stack alone would not create an independent address space.
