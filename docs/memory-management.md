# Simple memory management

The kernel provides a small first-fit heap allocator for early kernel-owned
allocations. It does not yet discover or manage the complete machine memory
map, provide virtual memory, or isolate processes.

## Physical layout

The current boot sector always reads eight sectors (4096 bytes) to physical
address `0x1000`. The allocator therefore reserves the 16 KiB range
`0x2000`–`0x5FFF` for its heap, immediately after the entire possible kernel
load region. The heap ends at `0x6000`.

The boot stack starts at `0x7C00` and grows down. This layout leaves a 7 KiB
gap between the heap and the initial stack pointer. It assumes conventional
memory is present through `0x6000`, as on the intended PC-compatible target.
The fixed range is a temporary boot-time memory map, not a BIOS-reported memory
map. Stack growth beyond the reserved gap could collide with the heap.

Each heap block has a four-byte header containing its payload size and state.
The header lives in the heap itself. Returned allocations are two-byte aligned.
The allocator uses first-fit search, splits sufficiently large free blocks, and
coalesces adjacent free blocks on release.

## API

Declared in `src/kernel/memory.h`:

| Function | Behavior |
| --- | --- |
| `memory_init()` | Initialize the heap. Must be called once before using the allocator; returns `false` if called again. |
| `kmalloc(size)` | Allocate `size` bytes, rounded up to an even size. Returns a null pointer for zero size, an invalid heap, or insufficient space. |
| `kfree(pointer)` | Free a pointer returned by `kmalloc`. Returns `false` for null, foreign, interior, or already-freed pointers. |
| `memory_get_stats(&stats)` | Fill the total, free, allocated payload bytes, and number of live allocations. Returns `false` for a null output, uninitialized heap, or invalid heap metadata. |

Example:

```cpp
MemoryStats stats;
if (!memory_get_stats(&stats))
    return;

void* buffer = kmalloc(128);
if (buffer) {
    // Use the buffer.
    kfree(buffer);
}
```

The caller owns allocations until they are freed. Allocations are not
zero-initialized. The allocator is single-threaded and not safe to call from an
interrupt handler or concurrently; there is no interrupt locking yet.

## Validation and limitations

The QEMU build runs a startup self-test covering allocation, distinct blocks,
payload access, reuse of a freed block, double-free rejection, coalescing, and
recovery of the largest allocation. It reports the result on the console.

This heap is intentionally fixed and small. A future memory-management step
should query the BIOS memory map, reserve firmware/device regions and the
active stack explicitly, and define ownership once kernel loading and process
memory are introduced.
