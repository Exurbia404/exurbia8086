# exurbia8086

## Build and run

```sh
./build.sh
```

The script requires the `ia16-elf` binutils and C++ compiler, plus QEMU. The
boot sector loads the kernel from sectors 2–9 at physical address `0x1000`,
then jumps to it. The kernel must therefore remain no larger than 4096 bytes.

Build products, including the floppy image, are written to `build/`.

The kernel starts with a VGA text console and a BIOS-backed keyboard prompt.
The prompt accepts a line of up to 79 characters; Backspace edits the current
line and Enter submits it. The current kernel echoes submitted lines as a
demonstration of the terminal input path.

The console API is declared in `src/kernel/console.h` (`kputchar`, `kputs`,
`kclear`, and `ksetcolor`). Keyboard input is declared in
`src/kernel/keyboard.h` (`kgetchar` and the editable, line-buffered `kgetline`).
The console uses 80x25 VGA text memory; keyboard input currently uses BIOS
interrupt `0x16`.

Enter `exit` on its own line to shut down the QEMU process. See
[`docs/console-and-terminal.md`](docs/console-and-terminal.md) for the full
feature guide, API behavior, and current limitations.

## Project layout

- `src/boot/` contains the BIOS boot sector.
- `src/kernel/` contains the freestanding kernel, VGA console, and keyboard
  input driver.
- `docs/` contains feature and API documentation.
- `linker/` contains linker scripts.
- `build.sh` assembles, links, creates the disk image, and launches QEMU.
