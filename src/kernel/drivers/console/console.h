#ifndef KERNEL_CONSOLE_H
#define KERNEL_CONSOLE_H

void console_init();
void kclear();
void ksetcolor(unsigned char foreground, unsigned char background);
void kputchar(char character);
void kputs(const char* string);

#endif
