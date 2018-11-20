#define _GNU_SOURCE

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Global Constants */
#define ADDRESS_PATH        argv[1]
#define BACKING_STORE_PATH  "./BACKING_STORE.bin"
#define OFFSET_MASK         0xFF
#define PAGE_SIZE           256
#define NUM_PAGES           256
#define FRAME_SIZE          256
#define NUM_FRAMES          256

/* Struct Type Prototypes */
typedef struct LogicalAddress LogicalAddress;
typedef struct Page Page;

/* LogicalAddress Function Prototypes */
LogicalAddress *newLogicalAddress(uint16_t);
uint16_t getLogicalAddress(LogicalAddress *);
uint8_t getLogicalAddressPageNumber(LogicalAddress *);
uint8_t getLogicalAddressOffset(LogicalAddress *);
void printLogicalAddress(FILE *, LogicalAddress *);

/* Page Function Prototypes */
Page *newPage(uint8_t);
int isValid(Page *);
uint8_t getPageFrameNumber(Page *);

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
    }
    free(line);

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


/********** Page Definitions **********/

typedef struct Page {
    int isValid;
    uint8_t frameNumber;
} Page;

Page *newPage(uint8_t frameNumber) {
    Page *page = malloc(sizeof(Page));
    page->isValid = 0;
    page->frameNumber = frameNumber;
}

int isValid(Page *page) {
    assert(page != 0);
    return page->isValid;
}

uint8_t getPageFrameNumber(Page *page) {
    assert(page != 0);
    return page->frameNumber;
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
        else                                modeString = "";
        fprintf(stderr, "Error: Cannot open %s", filename);
        // file mode not supported
        if (strcmp(mode, "") != 0) fprintf(stderr, " for %s!", modeString);
        printf("\n");
        exit(1);
    }
    return fp;
}
