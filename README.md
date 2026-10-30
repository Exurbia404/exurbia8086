# exurbia8086

## Build and run

```sh
./build.sh
```

The script requires the `ia16-elf` binutils and C++ compiler, plus QEMU. The
boot sector loads the kernel from sectors 2–17 at physical address `0x1000`,
then jumps to it. The kernel image must therefore remain no larger than
8192 bytes.

Build products, including the floppy image, are written to `build/`.

The kernel starts with a VGA text console and a BIOS-backed keyboard prompt.
The prompt accepts a line of up to 79 characters; Backspace edits the current
line and Enter submits it. The current kernel echoes submitted lines as a
demonstration of the terminal input path.

The console API is declared in `src/kernel/drivers/console/console.h`
(`kputchar`, `kputs`, `kclear`, and `ksetcolor`). Keyboard input is declared
in `src/kernel/drivers/keyboard/keyboard.h` (`kgetchar` and the editable,
line-buffered `kgetline`). The console uses 80x25 VGA text memory; keyboard
input currently uses BIOS interrupt `0x16`.

The kernel also has a fixed 12 KiB first-fit heap with `kmalloc`/`kfree`.
See [`docs/memory-management.md`](docs/memory-management.md) for its API,
layout, and current limitations.

The kernel has an `int 0x80` system-call interface for console read/write,
exit, and cooperative yield. See [`docs/system-calls.md`](docs/system-calls.md)
for the ABI and implemented calls.

The kernel also has a cooperative process model with separate stacks for up
to three kernel tasks. See [`docs/process-model.md`](docs/process-model.md) for
its API, scheduling behavior, and limitations.

Enter `exit` on its own line to shut down the QEMU process. See
[`docs/console-and-terminal.md`](docs/console-and-terminal.md) for the full
feature guide, API behavior, and current limitations.

## Project layout

- `src/boot/` contains the BIOS boot sector.
- `src/kernel/core/` contains kernel startup and the main kernel loop.
- `src/kernel/drivers/console/` contains VGA text output.
- `src/kernel/drivers/keyboard/` contains BIOS-backed keyboard input.
- `src/kernel/mm/` contains the fixed first-fit heap allocator.
- `src/kernel/process/` contains process control blocks and context switching.
- `src/kernel/syscall/` contains the software-interrupt entry and syscall ABI.
- `docs/` contains feature and API documentation.
- `linker/` contains linker scripts.
- `build.sh` assembles, links, creates the disk image, and launches QEMU.

## Planned locations for upcoming architecture tasks

These directories will be added when their implementation begins:

| Architecture task | Planned location |
| --- | --- |
| RAM/on-disk filesystem | `src/kernel/fs/` |
| User-mode shell | `src/user/shell/` |
| Executable loader | `src/kernel/loader/` |
| User programs and shared stdio | `src/user/programs/` and `src/user/libc/` |

Timer interrupts and native disk access belong under `src/kernel/drivers/`
when implemented. The current BIOS keyboard driver remains in its own driver
directory and can later be replaced or supplemented by a native 8042 driver.
