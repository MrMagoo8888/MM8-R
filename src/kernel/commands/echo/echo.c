#include "stdio.h"
#include "basicShell.h"

void echo(const char* input) {
    //kprintf(input); // debug tool
    if (input[4] == ' ') {
        kprintf("%s\n", input + 5);
    } else {
        kprintf("Usage: echo [text]\n");
    }
}