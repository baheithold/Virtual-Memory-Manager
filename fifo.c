#define _GNU_SOURCE

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Global Constants */
#define ADDRESS_PATH argv[1]

/* Function Prototypes */
FILE *openFile(char *, char *);


/* MAIN */
int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <filepath>\n", argv[0]);
        exit(1);
    }

    FILE *addresses = openFile(ADDRESS_PATH, "r");

    char *line = 0;
    size_t len = 0;
    while (getline(&line, &len, addresses) != -1) {
    }

    fclose(addresses);
    return 0;
}


/* Function Definitions */

FILE *openFile(char *filename, char *mode) {
    assert(filename != 0);
    assert(strcmp(filename, "") != 0);
    assert(mode != 0);
    assert(strcmp(mode, "") != 0);
    FILE *fp = fopen(filename, mode);
    // check if file was opened
    if (fp == 0) {
        char *modeString;
        // check for supported file mode
        if (strcmp(mode, "r") == 0)         modeString = "reading";
        else if (strcmp(mode, "rb") == 0)   modeString = "reading binary";
        else if (strcmp(mode, "w") == 0)    modeString = "writing";
        else if (strcmp(mode, "wb") == 0)   modeString = "writing binary";
        else                                modeString = "";
        printf("Error: Cannot open %s", filename);
        // file mode not supported
        if (strcmp(mode, "") != 0) printf(" for %s!", modeString);
        printf("\n");
        exit(1);
    }
    return fp;
}
