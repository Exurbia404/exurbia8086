#include "heap.h"

typedef unsigned short uint16_t;

static const uint16_t heap_start = 0x4000;
static const uint16_t heap_end = 0x7000;
static const uint16_t block_free = 0xFEE1;
static const uint16_t block_used = 0xA110;

struct Block {
    uint16_t size;
    uint16_t state;
};

static bool initialized;

static bool heap_is_valid() {
    uint16_t address = heap_start;

    while (address < heap_end) {
        if (heap_end - address < sizeof(Block))
            return false;

        Block* block = reinterpret_cast<Block*>(address);
        if ((block->state != block_free && block->state != block_used) ||
            !block->size ||
            (block->size & 1) != 0 ||
            block->size > heap_end - address - sizeof(Block))
            return false;

        address += sizeof(Block) + block->size;
    }

    return address == heap_end;
}

static Block* next_block(Block* block) {
    uint16_t address = reinterpret_cast<uint16_t>(block);
    address += sizeof(Block) + block->size;
    if (address == heap_end)
        return 0;
    return reinterpret_cast<Block*>(address);
}

static void coalesce_free_blocks() {
    Block* block = reinterpret_cast<Block*>(heap_start);
    while (block) {
        Block* next = next_block(block);
        if (next && block->state == block_free && next->state == block_free) {
            block->size += sizeof(Block) + next->size;
            continue;
        }
        block = next;
    }
}

bool memory_init() {
    if (initialized)
        return false;

    Block* first = reinterpret_cast<Block*>(heap_start);
    first->size = heap_end - heap_start - sizeof(Block);
    first->state = block_free;
    initialized = true;
    return true;
}

void* kmalloc(unsigned short size) {
    if (!initialized || !size || size > heap_end - heap_start - sizeof(Block) ||
        !heap_is_valid())
        return 0;

    if (size & 1)
        ++size;

    Block* block = reinterpret_cast<Block*>(heap_start);
    while (block) {
        if (block->state == block_free && block->size >= size) {
            uint16_t remaining = block->size - size;
            if (remaining >= sizeof(Block) + 2) {
                Block* split = reinterpret_cast<Block*>(
                    reinterpret_cast<uint16_t>(block) + sizeof(Block) + size
                );
                split->size = remaining - sizeof(Block);
                split->state = block_free;
                block->size = size;
            }

            block->state = block_used;
            return reinterpret_cast<unsigned char*>(block) + sizeof(Block);
        }
        block = next_block(block);
    }

    return 0;
}

bool kfree(void* pointer) {
    if (!initialized || !pointer || !heap_is_valid())
        return false;

    Block* block = reinterpret_cast<Block*>(heap_start);
    while (block) {
        void* payload = reinterpret_cast<unsigned char*>(block) + sizeof(Block);
        if (payload == pointer) {
            if (block->state != block_used)
                return false;

            block->state = block_free;
            coalesce_free_blocks();
            return true;
        }
        block = next_block(block);
    }

    return false;
}

bool memory_get_stats(MemoryStats* stats) {
    if (!initialized || !stats || !heap_is_valid())
        return false;

    stats->total_bytes = 0;
    stats->free_bytes = 0;
    stats->allocated_bytes = 0;
    stats->allocation_count = 0;

    Block* block = reinterpret_cast<Block*>(heap_start);
    while (block) {
        stats->total_bytes += block->size;
        if (block->state == block_free) {
            stats->free_bytes += block->size;
        } else {
            stats->allocated_bytes += block->size;
            ++stats->allocation_count;
        }
        block = next_block(block);
    }

    return true;
}
