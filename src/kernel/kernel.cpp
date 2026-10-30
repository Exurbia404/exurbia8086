typedef unsigned short uint16_t;

static inline void write_vga(uint16_t offset, uint16_t value) {
    __asm__ volatile ("movw %0, %%es:(%%di)" : : "r"(value), "D"(offset) : "memory");
}

extern "C" void _start() {
    __asm__ volatile ("movw $0xB800, %%ax\n\tmovw %%ax, %%es" ::: "ax", "memory");

    for (uint16_t i = 0; i < 80 * 25; i++)
        write_vga(i * 2, 0x0F00 | ' ');

    const char* msg = "Hello from C++ on 8086!";
    for (uint16_t i = 0; msg[i]; i++)
        write_vga(i * 2, 0x0F00 | msg[i]);

    while (1) {}
}
