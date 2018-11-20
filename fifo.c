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
typedef struct PageTable PageTable;
typedef struct PhysicalMemory PhysicalMemory;

/* LogicalAddress Function Prototypes */
LogicalAddress *newLogicalAddress(uint16_t);
uint16_t getLogicalAddress(LogicalAddress *);
uint8_t getLogicalAddressPageNumber(LogicalAddress *);
uint8_t getLogicalAddressOffset(LogicalAddress *);
void printLogicalAddress(FILE *, LogicalAddress *);

/* Page Function Prototypes */
Page *newPage(uint8_t);
int isPageValid(Page *);
void setPageValidation(Page *, int);
uint8_t getPageFrameNumber(Page *);
void setPageFrameNumber(Page *, uint8_t);

/* PageTable Function Prototypes */
PageTable *newPageTable(void);
Page *getPageFromPageTable(PageTable *, int);
void freePageTable(PageTable *);

/* PhysicalMemory Function Prototypes */
PhysicalMemory *newPhysicalMemory(void);
void freePhysicalMemory(PhysicalMemory *);
char *getPhysicalMemoryAtIndex(PhysicalMemory *, int);
int getPhysicalMemoryValue(PhysicalMemory *,int, int);

/* Function Prototypes */
FILE *openFile(char *, char *);


/*********** MAIN ***********/
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filepath>\n", argv[0]);
        exit(1);
    }

    // Open Files for reading
    FILE *addressesFile = openFile(ADDRESS_PATH, "r");
    FILE *backStoreFile = openFile(BACKING_STORE_PATH, "rb");

    // Create PageTable and PhysicalMemory
    PageTable *pageTable = newPageTable();
    PhysicalMemory *physicalMemory = newPhysicalMemory();

    int frameCounter = 0;
    int numPageFaults = 0;
    int numTranslated = 0;

    char *line = 0;
    size_t len = 0;
    while (getline(&line, &len, addressesFile) != -1) {
        uint32_t virtualAddress = atoi(line);
        LogicalAddress *logicalAddress = newLogicalAddress((uint16_t)virtualAddress);
        uint8_t currFrame = 0;
        Page *page = getPageFromPageTable(pageTable, getLogicalAddressOffset(logicalAddress));
        if (!isPageValid(page)) {
            long offset = getLogicalAddressPageNumber(logicalAddress) * PAGE_SIZE;
            fseek(backStoreFile, offset, SEEK_SET);
            fread(getPhysicalMemoryAtIndex(physicalMemory, frameCounter), 1, FRAME_SIZE, backStoreFile);
            setPageFrameNumber(page, frameCounter);
            setPageValidation(page, 1);
            currFrame = getPageFrameNumber(page);
            frameCounter++;
            numPageFaults++;
        }
        int physicalAddress = currFrame * FRAME_SIZE + getLogicalAddressOffset(logicalAddress);
        int value = getPhysicalMemoryValue(physicalMemory, currFrame, getLogicalAddressOffset(logicalAddress));
        printf("Virtual address: %d Physical address: %d Value: %d\n", virtualAddress, physicalAddress, value);
        numTranslated++;
    }

    printf("Number of Translated Addresses = %d\n", numTranslated);
    printf("Page Faults %d\n", numPageFaults);

    // Free memory
    freePageTable(pageTable);
    freePhysicalMemory(physicalMemory);
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
    return page;
}

int isPageValid(Page *page) {
    assert(page != 0);
    return page->isValid;
}

void setPageValidation(Page *page, int valid) {
    assert(page != 0);
    page->isValid = valid;
}

uint8_t getPageFrameNumber(Page *page) {
    assert(page != 0);
    return page->frameNumber;
}

void setPageFrameNumber(Page *page, uint8_t frameNumber) {
    assert(page != 0);
    page->frameNumber = frameNumber;
}


/********** PageTable Definitions **********/

typedef struct PageTable {
    Page **pages;
} PageTable;

PageTable *newPageTable(void) {
    PageTable *table = malloc(sizeof(PageTable));
    table->pages = malloc(sizeof(Page *) * NUM_PAGES);
    for (int i = 0; i < NUM_PAGES; ++i) {
        table->pages[i] = newPage(0);
    }
    return table;
}

Page *getPageFromPageTable(PageTable *table, int index) {
    assert(table != 0);
    assert(index >= 0);
    return table->pages[index];
}

void freePageTable(PageTable *table) {
    assert(table != 0);
    for (int i = 0; i < NUM_PAGES; ++i) {
        free(table->pages[i]);
    }
    free(table->pages);
    free(table);
}


/********** PhysicalMemory Definitions **********/

typedef struct PhysicalMemory {
    char **memory;
} PhysicalMemory;

PhysicalMemory *newPhysicalMemory(void) {
    PhysicalMemory *mem = malloc(sizeof(PhysicalMemory));
    mem->memory = malloc(sizeof(char *) * NUM_FRAMES);
    for (int i = 0; i < NUM_FRAMES; ++i) {
        mem->memory[i] = malloc(sizeof(char) * FRAME_SIZE);
    }
    return mem;
}

char *getPhysicalMemoryAtIndex(PhysicalMemory *mem, int index) {
    assert(mem != 0);
    return mem->memory[index];
}

int getPhysicalMemoryValue(PhysicalMemory *mem, int i, int j) {
    assert(mem != 0);
    assert(i >= 0);
    assert(j >= 0);
    return mem->memory[i][j];
}

void freePhysicalMemory(PhysicalMemory *mem) {
    assert(mem != 0);
    for (int i = 0; i < NUM_FRAMES; ++i) {
        free(mem->memory[i]);
    }
    free(mem->memory);
    free(mem);
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
