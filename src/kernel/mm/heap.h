#ifndef KERNEL_MEMORY_H
#define KERNEL_MEMORY_H

struct MemoryStats {
    unsigned short total_bytes;
    unsigned short free_bytes;
    unsigned short allocated_bytes;
    unsigned short allocation_count;
};

bool memory_init();
void* kmalloc(unsigned short size);
bool kfree(void* pointer);
bool memory_get_stats(MemoryStats* stats);

#endif
