#define _GNU_SOURCE

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Global Constants */
#define ADDRESS_PATH        argv[1]
#define BACKING_STORE_PATH  "./BACKING_STORE.bin"
#define PAGE_MASK           0xFFFF
#define OFFSET_MASK         0xFF
#define PAGE_SIZE           256
#define NUM_PAGES           256
#define FRAME_SIZE          256
#define NUM_FRAMES          128
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
int updateTLB(TLB *, int, LogicalAddress *, int);
void freeTLB(TLB *);

/* Function Prototypes */
FILE *openFile(char *, char *);
int translateLogicalToPhysicalAddress(uint8_t, LogicalAddress *);
void handlePageFault(Page *, LogicalAddress *, PhysicalMemory *, int, FILE *);


/*********** MAIN ***********/
int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <filepath>\n", argv[0]);
        exit(1);
    }

    // Open Files for reading
    FILE *addressesFile = openFile(ADDRESS_PATH, "r");
    FILE *backStoreFile = openFile(BACKING_STORE_PATH, "rb");

    // Create PageTable, PhysicalMemory, and TLB
    PageTable *pageTable = newPageTable();
    PhysicalMemory *physicalMemory = newPhysicalMemory();
    TLB *tlb = newTLB();

    // Counters
    int frameCounter = 0;
    int TLBCounter = 0;
    int numPageFaults = 0;
    int numTranslated = 0;
    int TLBhits = 0;

    // Perform Translations
    char *line = 0;
    size_t len = 0;
    while (getline(&line, &len, addressesFile) != -1) {
        uint8_t currFrame = 0;
        // Get Logical Address from Addresses File
        uint32_t virtualAddress = atoi(line);
        LogicalAddress *logicalAddress = newLogicalAddress((uint16_t)virtualAddress);
        // Check TLB for page
        int8_t TLBframe = TLBlookup(tlb, getLogicalAddressPageNumber(logicalAddress));
        if (TLBframe != -1) {
            // TLB Hit
            currFrame = TLBframe;
            TLBhits++;
        }
        else {
            Page *page = getPageFromPageTable(pageTable, getLogicalAddressPageNumber(logicalAddress));
            if (!isPageValid(page)) {
                // Page Fault
                handlePageFault(page, logicalAddress, physicalMemory, frameCounter, backStoreFile);
                frameCounter++;
                numPageFaults++;
            }
            // Get frame and update TLB
            currFrame = getPageFrameNumber(page);
            TLBCounter = updateTLB(tlb, TLBCounter, logicalAddress, currFrame);
        }
        int physicalAddress = translateLogicalToPhysicalAddress(currFrame, logicalAddress);
        int value = getPhysicalMemoryValue(physicalMemory, currFrame, getLogicalAddressOffset(logicalAddress));
        printf("Virtual address: %d Physical address: %d Value: %d\n", virtualAddress, physicalAddress, value);
        numTranslated++;
        free(logicalAddress);
    }

    // Free memory
    freePageTable(pageTable);
    freePhysicalMemory(physicalMemory);
    freeTLB(tlb);
    free(line);

    // Close files
    fclose(addressesFile);
    fclose(backStoreFile);

    // Display Statistics
    printf("Number of Translated Addresses = %d\n", numTranslated);
    printf("Page Faults = %d\n", numPageFaults);
    printf("Page Fault Rate = %.3f\n", (float)(numPageFaults) / numTranslated);
    printf("TLB Hits = %d\n", TLBhits);
    printf("TLB Hit Rate = %.3f\n", (float)(TLBhits) / numTranslated);

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
    uint8_t msb = (n & PAGE_MASK) >> 8;
    uint8_t lsb = n & OFFSET_MASK;
    addr->pageNumber = msb;
    addr->offset = lsb;
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
    int lastUsed;
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

int getPhysicalMemoryValue(PhysicalMemory *mem, int frameNumber, int offset) {
    assert(mem != 0);
    assert(frameNumber >= 0);
    assert(offset >= 0);
    return mem->memory[frameNumber][offset];
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

int updateTLB(TLB *tlb, int counter, LogicalAddress *logicalAddress, int frame) {
    assert(tlb != 0);
    assert(counter >= 0);
    assert(logicalAddress != 0);
    setTLBPageAtIndex(tlb, counter, getLogicalAddressPageNumber(logicalAddress));
    setTLBFrameAtIndex(tlb, counter, frame);
    return ++counter % TLB_SIZE;
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

int translateLogicalToPhysicalAddress(uint8_t frame, LogicalAddress *logicalAddress) {
    assert(logicalAddress != 0);
    return frame * FRAME_SIZE + getLogicalAddressOffset(logicalAddress);
}

void handlePageFault(Page *page, LogicalAddress *la, PhysicalMemory *mem, int frame, FILE *backingStore) {
    assert(page != 0);
    assert(la != 0);
    assert(mem != 0);
    assert(frame >= 0);
    assert(backingStore != 0);
    long offset = getLogicalAddressPageNumber(la) * PAGE_SIZE;
    fseek(backingStore, offset, SEEK_SET);
    fread(getPhysicalMemoryAtIndex(mem, frame), 1, FRAME_SIZE, backingStore);
    setPageFrameNumber(page, frame);
    setPageValidation(page, 1);
}
