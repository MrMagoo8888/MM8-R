#include "basicShell.h"

#include "stdio.h"
#include "memory.h"
#include "string.h"

void helpHandle() {
    kprintf("\n================================================\n");
    kprintf("Avaliable Commands:\n\n");

    kprintf(" - help: Shows this message\n");

    kprintf("\n================================================\n\n");
}

void commandDispatcher(const char* input) {
    if (strcmp(input, "help") == 0) {
        helpHandle();
    }
}

char inputBuff[256];
void mainShell() {
    int inShell = 1;
    while (inShell == 1) {
        kprintf("> ");

        kgets(inputBuff, sizeof(inputBuff));
        //kprintf("sanity1");

        commandDispatcher(inputBuff);

    }
    return;
}