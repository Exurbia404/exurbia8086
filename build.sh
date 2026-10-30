#!/bin/bash
set -e

ROOT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
BUILD_DIR="$ROOT_DIR/build"
AS="${AS:-ia16-elf-as}"
CC="${CC:-ia16-elf-g++}"
LD="${LD:-ia16-elf-ld}"

CFLAGS=(
    -ffreestanding -nostdlib -fno-exceptions -fno-rtti
    -march=i8086 -mtune=i8086 -mcmodel=tiny -Os -Wall
    -DEXURBIA_QEMU
)

mkdir -p "$BUILD_DIR"

echo "Assembling boot sector..."
"$AS" -o "$BUILD_DIR/boot.o" "$ROOT_DIR/src/boot/boot.s"
"$LD" -Ttext 0x7C00 --oformat binary -o "$BUILD_DIR/boot.bin" "$BUILD_DIR/boot.o"

echo "Compiling kernel..."
"$CC" "${CFLAGS[@]}" -c "$ROOT_DIR/src/kernel/core/kernel.cpp" -o "$BUILD_DIR/kernel.o"
"$CC" "${CFLAGS[@]}" -c "$ROOT_DIR/src/kernel/drivers/console/console.cpp" -o "$BUILD_DIR/console.o"
"$CC" "${CFLAGS[@]}" -c "$ROOT_DIR/src/kernel/drivers/keyboard/keyboard.cpp" -o "$BUILD_DIR/keyboard.o"
"$CC" "${CFLAGS[@]}" -c "$ROOT_DIR/src/kernel/mm/heap.cpp" -o "$BUILD_DIR/heap.o"
"$CC" "${CFLAGS[@]}" -c "$ROOT_DIR/src/kernel/syscall/syscall.cpp" -o "$BUILD_DIR/syscall.o"
"$AS" -o "$BUILD_DIR/syscall-entry.o" "$ROOT_DIR/src/kernel/syscall/entry.s"
"$LD" -T "$ROOT_DIR/linker/kernel.ld" --oformat binary \
    -o "$BUILD_DIR/kernel.bin" "$BUILD_DIR/kernel.o" \
    "$BUILD_DIR/console.o" "$BUILD_DIR/keyboard.o" "$BUILD_DIR/heap.o" \
    "$BUILD_DIR/syscall.o" "$BUILD_DIR/syscall-entry.o"

if [ "$(wc -c < "$BUILD_DIR/boot.bin")" -ne 512 ]; then
    echo "Error: boot sector must be exactly 512 bytes." >&2
    exit 1
fi

if [ "$(wc -c < "$BUILD_DIR/kernel.bin")" -gt 4096 ]; then
    echo "Error: kernel exceeds the 8-sector limit (4096 bytes)." >&2
    exit 1
fi

echo "Creating disk image..."
cat "$BUILD_DIR/boot.bin" "$BUILD_DIR/kernel.bin" > "$BUILD_DIR/myos.img"
truncate -s 16K "$BUILD_DIR/myos.img"

echo "Starting QEMU..."
if qemu-system-i386 \
    -drive "file=$BUILD_DIR/myos.img,format=raw,if=floppy" \
    -device isa-debug-exit,iobase=0xf4,iosize=0x01 \
    -boot a; then
    :
else
    qemu_status=$?
    if [ "$qemu_status" -eq 33 ]; then
        echo "Guest requested shutdown."
    else
        echo "QEMU failed with exit status $qemu_status." >&2
        exit "$qemu_status"
    fi
fi
