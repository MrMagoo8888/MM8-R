#include "basicShell.h"

#include "stdio.h"
#include "memory.h"
#include "string.h"

void helpHandle() {
    kprintf("================================================\n");
    kprintf("Avaliable Commands:\n\n");

    kprintf(" - help: Shows this message\n");

    kprintf("\n================================================\n");
}

void commandDispatcher(const char* input) {
    if (strcmp(input, "help") == 0) {
        helpHandle();
    }
}