#define _GNU_SOURCE

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Global Constants */
#define ADDRESS_PATH argv[1]
#define OFFSET_MASK 0xFF

/* Struct Type Prototypes */
typedef struct LogicalAddress LogicalAddress;

/* LogicalAddress Function Prototypes */
LogicalAddress *newLogicalAddress(uint16_t);
uint16_t getLogicalAddress(LogicalAddress *);
uint8_t getLogicalAddressPageNumber(LogicalAddress *);
uint8_t getLogicalAddressOffset(LogicalAddress *);
void printLogicalAddress(FILE *, LogicalAddress *);

/* Function Prototypes */
FILE *openFile(char *, char *);


/*********** MAIN ***********/
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filepath>\n", argv[0]);
        exit(1);
    }

    FILE *addressesFile = openFile(ADDRESS_PATH, "r");

    char *line = 0;
    size_t len = 0;
    while (getline(&line, &len, addressesFile) != -1) {
        LogicalAddress *la = newLogicalAddress((uint16_t)atoi(line));
        printLogicalAddress(stdout, la);
    }

    fclose(addressesFile);
    return 0;
}


/********** LogicalAddress Definitions **********/

typedef struct LogicalAddress {
    uint16_t address;
    uint8_t pageNumber;
    uint8_t offset;
} LogicalAddress;

LogicalAddress *newLogicalAddress(uint16_t n) {
    assert(n > 0);
    LogicalAddress *addr = malloc(sizeof(LogicalAddress));
    addr->address = n;
    uint16_t msb = n >> 8;
    uint16_t lsb = n & OFFSET_MASK;
    addr->pageNumber = (uint8_t)msb;
    addr->offset = (uint8_t)lsb;
    return addr;
}

uint16_t getLogicalAddress(LogicalAddress *addr) {
    assert(addr != 0);
    return addr->address;
}

uint8_t getLogicalAddressPageNumber(LogicalAddress *addr) {
    assert(addr != 0);
    return addr->pageNumber;
}

uint8_t getLogicalAddressOffset(LogicalAddress *addr) {
    assert(addr != 0);
    return addr->offset;
}

void printLogicalAddress(FILE *fp, LogicalAddress *addr) {
    assert(addr != 0);
    fprintf(fp, "Address: %d Page Number: %d Offset: %d\n", addr->address, addr->pageNumber, addr->offset);
}


/*********** Function Definitions ***********/

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
        fprintf(stderr, "Error: Cannot open %s", filename);
        // file mode not supported
        if (strcmp(mode, "") != 0) fprintf(stderr, " for %s!", modeString);
        printf("\n");
        exit(1);
    }
    return fp;
}
