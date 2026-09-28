#ifndef KEYBOARD_H
#define KEYBOARD_H

void keyboard_initialize(void);
void keyboard_irq_handler(void);
int keyboard_getchar(void);
void kgets(char* buffer, int size);
int kgetch();

#endif