# exurbia8086

## Build and run

```sh
./build.sh
```

The script requires the `ia16-elf` binutils and C++ compiler, plus QEMU. The
boot sector loads the kernel from sectors 2–9 at physical address `0x1000`,
then jumps to it. The kernel must therefore remain no larger than 4096 bytes.

Build products, including the floppy image, are written to `build/`.

## Project layout

- `src/boot/` contains the BIOS boot sector.
- `src/kernel/` contains the freestanding kernel.
- `linker/` contains linker scripts.
- `build.sh` assembles, links, creates the disk image, and launches QEMU.
