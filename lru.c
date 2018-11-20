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
#define TLB_SIZE            16

/* Struct Type Prototypes */
typedef struct LogicalAddress LogicalAddress;
typedef struct Page Page;
typedef struct PageTable PageTable;
typedef struct PhysicalMemory PhysicalMemory;
typedef struct TLBNode TLBNode;
typedef struct TLB TLB;

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

/* TLBNode Function Prototypes */
TLBNode *newTLBNode(uint8_t, uint8_t);
uint8_t getTLBNodePageNumber(TLBNode *);
void setTLBNodePageNumber(TLBNode *, uint8_t);
uint8_t getTLBNodeFrameNumber(TLBNode *);
void setTLBNodeFrameNumber(TLBNode *, uint8_t);

/* TLB Function Prototypes */
TLB *newTLB(void);
void setTLBPageAtIndex(TLB *, int, uint8_t);
void setTLBFrameAtIndex(TLB *, int, uint8_t);
int8_t TLBlookup(TLB *, uint8_t);
void freeTLB(TLB *);

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
    TLB *tlb = newTLB();

    int frameCounter = 0;
    int TLBCounter = 0;
    int numPageFaults = 0;
    int numTranslated = 0;
    int TLBhits = 0;

    char *line = 0;
    size_t len = 0;
    while (getline(&line, &len, addressesFile) != -1) {
        // Convert virtual address to logical address
        uint32_t virtualAddress = atoi(line);
        LogicalAddress *logicalAddress = newLogicalAddress((uint16_t)virtualAddress);
        // Check TLB for page
        int8_t TLBframe = TLBlookup(tlb, getLogicalAddressPageNumber(logicalAddress));
        uint8_t currFrame = 0;
        if (TLBframe != -1) {
            // TLB Hit
            currFrame = TLBframe;
            TLBhits++;
        }
        else {
            Page *page = getPageFromPageTable(pageTable, getLogicalAddressPageNumber(logicalAddress));
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
            currFrame = getPageFrameNumber(page);
            setTLBPageAtIndex(tlb, TLBCounter, getLogicalAddressPageNumber(logicalAddress));
            setTLBFrameAtIndex(tlb, TLBCounter, currFrame);
            TLBCounter = (TLBCounter + 1) % TLB_SIZE;
        }
        int physicalAddress = currFrame * FRAME_SIZE + getLogicalAddressOffset(logicalAddress);
        int value = getPhysicalMemoryValue(physicalMemory, currFrame, getLogicalAddressOffset(logicalAddress));
        printf("Virtual address: %d Physical address: %d Value: %d\n", virtualAddress, physicalAddress, value);
        numTranslated++;
        free(logicalAddress);
    }

    // Display Statistics
    printf("Number of Translated Addresses = %d\n", numTranslated);
    printf("Page Faults %d\n", numPageFaults);
    printf("Page Fault Rate = %.3f\n", (float)(numPageFaults) / numTranslated);
    printf("TLB Hits = %d\n", TLBhits);
    printf("TLB Hit Rate = %.3f\n", (float)(TLBhits) / numTranslated);

    // Free memory
    freePageTable(pageTable);
    freePhysicalMemory(physicalMemory);
    freeTLB(tlb);
    free(line);

    // Close files
    fclose(addressesFile);
    fclose(backStoreFile);
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


/********** TLBNode Definitions **********/

typedef struct TLBNode {
    uint8_t pageNumber;
    uint8_t frameNumber;
} TLBNode;

TLBNode *newTLBNode(uint8_t page, uint8_t frame) {
    TLBNode *n = malloc(sizeof(TLBNode));
    n->pageNumber = page;
    n->frameNumber = frame;
    return n;
}

uint8_t getTLBNodePageNumber(TLBNode *n) {
    assert(n != 0);
    return n->pageNumber;
}

void setTLBNodePageNumber(TLBNode *n, uint8_t page) {
    assert(n != 0);
    n->pageNumber = page;
}

uint8_t getTLBNodeFrameNumber(TLBNode *n) {
    assert(n != 0);
    return n->frameNumber;
}

void setTLBNodeFrameNumber(TLBNode *n, uint8_t frame) {
    assert(n != 0);
    n->frameNumber = frame;
}


/********** TLBNode Definitions **********/

typedef struct TLB {
    TLBNode **nodes;
} TLB;

TLB *newTLB(void) {
    TLB *tlb = malloc(sizeof(TLB));
    tlb->nodes = malloc(sizeof(TLBNode *) * TLB_SIZE);
    for (int i = 0; i < TLB_SIZE; ++i) {
        tlb->nodes[i] = newTLBNode(-1, -1);
    }
    return tlb;
}

void setTLBPageAtIndex(TLB *tlb, int index, uint8_t page) {
    assert(tlb != 0);
    assert(index >= 0);
    setTLBNodePageNumber(tlb->nodes[index], page);
}

void setTLBFrameAtIndex(TLB *tlb, int index, uint8_t frame) {
    assert(tlb != 0);
    assert(index >= 0);
    setTLBNodeFrameNumber(tlb->nodes[index], frame);
}

int8_t TLBlookup(TLB *tlb, uint8_t page) {
    assert(tlb != 0);
    for (int i = 0; i < TLB_SIZE; ++i) {
        if (getTLBNodePageNumber(tlb->nodes[i]) == page) {
            return getTLBNodeFrameNumber(tlb->nodes[i]);
        }
    }
    return -1;
}

void freeTLB(TLB *tlb) {
    assert(tlb != 0);
    for (int i = 0; i < TLB_SIZE; ++i) {
        free(tlb->nodes[i]);
    }
    free(tlb->nodes);
    free(tlb);
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