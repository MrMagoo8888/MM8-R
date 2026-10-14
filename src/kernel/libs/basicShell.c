#include "basicShell.h"

#include "stdio.h"
#include "memory.h"
#include "string.h"
#include "splash.h"

// commands
#include "echo.h"
#include "disclaimer.h"


void helpHandle() {
    kprintf("\n================================================\n");
    kprintf("Avaliable Commands:\n\n");

    kprintf(" - help: Shows this message\n");

    kprintf("\n================================================\n\n");
}

void commandDispatcher(const char* input) {
    if (strcmp(input, "help") == 0) {
        helpHandle();
    } else if (memcmp(input, "echo ", 5) == 0) {
        echo(input); 
    } else if (strcmp(input, "disclaimer") == 0) {
        disclaimer();
    } else {
        kprintf("Unknown command: %s\n", input);
    }
}

char inputBuff[256];
void mainShell() {
    int inShell = 1;
    clrscr();
    splash();
    while (inShell == 1) {
        kprintf("> ");

        kgets(inputBuff, sizeof(inputBuff));
        //kprintf("sanity1");

        commandDispatcher(inputBuff);

    }
    return;
}