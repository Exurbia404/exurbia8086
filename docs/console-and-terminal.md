# Console and terminal capabilities

This document describes the console and keyboard features currently included
in the kernel. The project is still a small real-mode kernel, not a Linux-like
shell or a complete operating system.

## What the booted kernel does

After boot, the kernel initializes an 80-column by 25-row VGA text console and
prints a welcome message. It then displays a `>` prompt and waits for a line of
keyboard input.

- Printable characters are echoed as they are typed.
- Backspace removes the last character in the current input line.
- Enter submits the line.
- Submitted lines are echoed with their character count.
- Entering exactly `exit` (lowercase, with no surrounding spaces) requests
  shutdown of the QEMU process.

Other input is currently echoed rather than interpreted as a command. There is
no process model, user account, privilege system, or shell yet; the prompt is
the kernel's own input loop.

## VGA console

The console writes directly to color VGA text memory at physical address
`0xB8000` and updates the hardware text cursor. It supports:

- 80x25 character cells.
- Foreground and background colors.
- Newline, carriage return, tab, and backspace handling.
- Scrolling when output reaches the bottom row.
- Clearing the screen.

Each cell stores an ASCII character and an 8-bit color attribute. The
foreground and background arguments to `ksetcolor` use the standard VGA color
indices `0` through `15`; values outside that range are masked to four bits.
Blinking and bright-background behavior are not configured separately.

### Console API

Declared in `src/kernel/console.h`:

| Function | Behavior |
| --- | --- |
| `console_init()` | Set the default light-gray-on-black color and clear the screen. |
| `kclear()` | Fill the screen with spaces in the current color and move the cursor to the top-left. |
| `ksetcolor(foreground, background)` | Select VGA foreground and background color indices for subsequent output. |
| `kputchar(character)` | Write one character; handles `\n`, `\r`, `\t`, and `\b` specially. |
| `kputs(string)` | Write a null-terminated string using `kputchar`. |

These are direct kernel functions, not system calls, and are not yet available
to separate user programs.

## Keyboard input

Input currently uses BIOS interrupt `0x16`, function `0`, so the BIOS supplies
translated character values according to its active keyboard layout. Calls
block until a key is available. This depends on BIOS services remaining usable
after the kernel starts; it is intended as an early development input path, not
a native keyboard driver.

### Keyboard API

Declared in `src/kernel/keyboard.h`:

| Function | Behavior |
| --- | --- |
| `kgetchar()` | Wait for and return the next nonzero character from the BIOS. Extended keys without an ASCII character are ignored. |
| `kgetline(buffer, capacity)` | Read and echo a line, allowing Backspace editing. Return its length and add a null terminator when Enter is pressed. |

`kgetline` stores at most `capacity - 1` characters so there is room for the
terminating null byte. It ignores additional printable input while full. A null
buffer or zero capacity returns zero without reading input. The current demo
uses an 80-byte buffer, allowing up to 79 characters per line.

## Exit behavior

The `exit` command is recognized only when it is the entire submitted line. In
the QEMU build, the kernel writes value `0x10` to QEMU's `isa-debug-exit` device
at I/O port `0xF4`; `build.sh` attaches that device so QEMU closes when
requested. QEMU reports this guest exit as host status `33`, which the build
script recognizes as a successful guest shutdown.

This is emulator-specific: real hardware has no host QEMU process for the
kernel to close. When built without the `EXURBIA_QEMU` option, `exit` halts the
CPU instead.

## Building and trying it

Run `./build.sh` from any working directory. It builds the floppy image in
`build/` and launches QEMU. Type a line, use Backspace to edit it, and press
Enter to submit. Enter `exit` to close QEMU normally.

The boot sector currently loads only the first 4096 bytes of the kernel.
`build.sh` checks this limit as well as the 512-byte boot-sector size.
