#include "basicShell.h"

#include "stdio.h"
#include "memory.h"
#include "string.h"

void helpHandle() {
    kprintf("Avaliable Commands:\n\n");

    kprintf(" - help: Shows this message");
}

void commandDispatcher(const char* input) {
    if (strcmp(input, "help") == 0) {
        helpHandle();
    }
}