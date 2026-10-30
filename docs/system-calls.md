# System-call interface

The kernel exposes a small real-mode software-interrupt ABI through interrupt
`0x80`. The interrupt handler saves the caller's registers, dispatches the call,
returns its result in `AX`, restores the other registers, and returns with
`iret`.

## Calling convention

| Register | Input |
| --- | --- |
| `AX` | System-call number |
| `BX` | First argument |
| `CX` | Second argument |
| `DX` | Third argument |

The signed 16-bit result is returned in `AX`. Nonnegative values are successful
results; negative values are errors. Calls preserve registers other than `AX`
and may modify condition flags.

The kernel C++ wrappers in `src/kernel/syscall/syscall.h` issue `int 0x80`
using this convention. Their buffer and path arguments are 16-bit offsets,
not C++ pointers; callers must explicitly convert pointers in the segment-zero
kernel layout to offsets:

| Wrapper | Arguments | Result |
| --- | --- | --- |
| `syscall_read(fd, buffer_offset, count)` | `fd` in `BX`, buffer offset in `CX`, count in `DX` | Number of bytes read |
| `syscall_write(fd, buffer_offset, count)` | `fd` in `BX`, buffer offset in `CX`, count in `DX` | Number of bytes written |
| `syscall_open(path_offset, flags)` | Path offset in `BX`, flags in `CX` | Not implemented yet |
| `syscall_close(fd)` | `fd` in `BX` | Not implemented yet |
| `syscall_fork()` | No arguments | Not implemented yet |
| `syscall_exit(status)` | Status in `BX` | Does not return |

Pointer arguments are 16-bit offsets in segment zero, matching the current
kernel's real-mode data layout. The current implementation rejects ranges that
would wrap past the end of that segment, but cannot verify that an otherwise
in-range pointer refers to valid or caller-owned memory.

The defined syscall numbers are `SYS_READ` (0), `SYS_WRITE` (1), `SYS_OPEN` (2),
`SYS_CLOSE` (3), `SYS_EXIT` (4), and `SYS_FORK` (5). `open`, `close`, and `fork`
are reserved in the ABI but currently return `SYS_ERROR_NOT_IMPLEMENTED`
(`-1`). They will become useful when filesystem and process support exist.

## Current behavior

File descriptors are currently fixed:

- `0`: keyboard input.
- `1`: console output.
- `2`: console error output.

`read` accepts descriptor 0. It uses the BIOS keyboard line editor, echoes
typed input, and returns the submitted line including its newline. Lines hold
up to 79 typed characters plus the newline. If a caller's buffer is smaller
than a line, the rest of that line remains buffered for the next read.

`write` accepts descriptors 1 and 2, sending each byte through the VGA console.
Console control characters retain the behavior described in
[console-and-terminal.md](console-and-terminal.md). Read and write requests
larger than 32767 bytes are rejected because successful results use a signed
16-bit return value.

Defined errors are:

| Value | Meaning |
| --- | --- |
| `SYS_ERROR_NOT_IMPLEMENTED` (`-1`) | Unknown syscall or reserved call without an implementation |
| `SYS_ERROR_BAD_FILE_DESCRIPTOR` (`-2`) | Descriptor is not supported by this operation |
| `SYS_ERROR_INVALID_ARGUMENT` (`-3`) | Invalid pointer/count combination or request too large |

`SYS_EXIT` does not return. QEMU builds use the `isa-debug-exit` device to close
the emulator. Without that device, the kernel halts the CPU. The status
argument is reserved but currently ignored.

## Scope and limitations

This is an early kernel ABI, not a Linux-compatible syscall table. The current
prompt calls the wrappers from kernel code to exercise the interrupt path;
there are not yet separate user programs or a user/kernel privilege boundary.
Real-mode code can access kernel memory and invoke any interrupt, so syscall
pointers cannot be safely isolated or validated against a user address space.
The implementation is single-threaded and has no scheduler or filesystem.

The QEMU startup self-test checks console writes, invalid descriptors and
arguments, and reserved calls. The interactive prompt also uses `read`,
`write`, and `exit` through the interrupt interface.
