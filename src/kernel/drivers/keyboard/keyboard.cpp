#include "keyboard.h"
#include "../console/console.h"

char kgetchar() {
    for (;;) {
        unsigned short key;
        __asm__ volatile (
            "int $0x16"
            : "=a"(key)
            : "0"(0)
            : "cc", "memory"
        );

        const char character = static_cast<char>(key & 0x00FF);
        if (character)
            return character;
    }
}

unsigned short kgetline(char* buffer, unsigned short capacity) {
    if (!buffer || !capacity)
        return 0;

    unsigned short length = 0;
    for (;;) {
        const char character = kgetchar();

        if (character == '\r' || character == '\n') {
            kputchar('\n');
            buffer[length] = '\0';
            return length;
        }

        if (character == '\b' || character == 0x7F) {
            if (length) {
                --length;
                kputchar('\b');
            }
        } else if (character >= ' ' && length < capacity - 1) {
            buffer[length++] = character;
            kputchar(character);
        }
    }
}
