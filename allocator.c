#include <stdio.h>
#include <stdlib.h>
#include "bheader.c"
#include <stdbool.h>
#include <ctype.h>
 
#ifndef HEXDUMP_COLS
#define HEXDUMP_COLS 16
#endif
 
void hexdump(void *mem, unsigned int len)
{
        unsigned int i, j;
        
        for(i = 0; i < len + ((len % HEXDUMP_COLS) ? (HEXDUMP_COLS - len % HEXDUMP_COLS) : 0); i++)
        {
                /* print offset */
                if(i % HEXDUMP_COLS == 0)
                {
                        printf("0x%06x: ", i);
                }
 
                /* print hex data */
                if(i < len)
                {
                        printf("%02x ", 0xFF & ((char*)mem)[i]);
                }
                else /* end of block, just aligning for ASCII dump */
                {
                        printf("   ");
                }
                
                /* print ASCII dump */
                if(i % HEXDUMP_COLS == (HEXDUMP_COLS - 1))
                {
                        for(j = i - (HEXDUMP_COLS - 1); j <= i; j++)
                        {
                                if(j >= len) /* end of block, not really printing */
                                {
                                        putchar(' ');
                                }
                                else if(isprint(((char*)mem)[j])) /* printable char */
                                {
                                        putchar(0xFF & ((char*)mem)[j]);        
                                }
                                else /* other char */
                                {
                                        putchar('.');
                                }
                        }
                        putchar('\n');
                }
        }
}


//size of available space
#define ALLOCSIZE 1000
// starting block of free memory
static bheader h = {0,0,0};
// starting full block of memory
static bheader f = {0,0,0};
// size of a block header
size_t BHDRSIZE = sizeof(h);

//storage for alloc
static char allocbuf[ALLOCSIZE];

//next available position
static char *allocp = allocbuf;

void printfull(bheader start){
    bheader* block = &start;
    printf("\n\n****** PRINT %s BLOCKS ******\n\n",  block->next == f.next ? "FULL" : "EMPTY");
    if(block->next == 0){
        printf("NO BLOCKS.\n\n");
        return;
    }
    char* curbyte;
    for(int i = 0; block!=0; i++){
        printf("*** HEADER %d ***\n", i);
        int j;
        for(j = 0; j < BHDRSIZE; j++){
            curbyte = (char*)(block) + j;
            printf("%p : %x\n", curbyte, *curbyte);
        }
        printf("*** DATA ***\n");
        for(;j < BHDRSIZE+block->size; j++){
            curbyte = (char*)(block) + j;
            printf("%p : %c\n", curbyte, *curbyte);
        }
        block = block->next;
    }
    printf("\n");
}

bool overwrites(bheader* block){
    printf("\nIN OVERWRITES\n\n");
    bheader* start = block;
    bheader* end = (bheader*)((char*)block + BHDRSIZE + block->size);
    printf("End of block in overwrites: %p\n", end);
    bheader* checkedstart = &f;
    bheader* checkedend = (bheader*)((char*)checkedstart - 1 + BHDRSIZE + checkedstart->size);
    while (checkedstart != 0){
        // if start or end of the new block between start and end of a full block
        if((checkedstart <= start && start <= checkedend) || (checkedstart <= end && end < checkedend)){
            printf("\n!!! ERROR: block at %p tried to modify memory of the block at %p !!!\n", block, checkedstart);
            printf("saved block's registers: %p - %p\n", checkedstart, checkedend);
            printf("Intrusive block's registers: %p - %p\n", start, end);
            printfull(f);
            return true;
        }
        checkedstart = checkedstart->next;
        if(checkedstart != 0)
            checkedend = (bheader*)((char*)checkedstart - 1 + BHDRSIZE + checkedstart->size); 
    }
    checkedstart = &h;
    checkedend = (bheader*)((char*)checkedstart - 1 + BHDRSIZE + checkedstart->size);
    while (checkedstart != 0){
         if((checkedstart <= start && start < checkedend) || (checkedstart <= end && end < checkedend)){
            printf("\n !!! ERROR: block at %p tried to modify memory of the block at %p.\n", block, checkedstart);
            printf("saved block's registers: %p - %p\n", checkedstart, checkedend);
            printf("Intrusive block's registers: %p - %p\n", start, end);
            return true;
        }
        checkedstart = checkedstart->next;
        if(checkedstart != 0)
            checkedend = (bheader*)((char*)checkedstart - 1 + BHDRSIZE + checkedstart->size);
    }
    printf("block at %p didn't try to overwrite any other blocks.\n", block);
    return false;
}

bool inserttoblocklist(bheader* block, bheader* listhdr){
    if(block == 0 || block == &f || block == &h || overwrites(block)){
        printf("Insertion error: invalid pointer.\n");
        return false;
    }
    bheader* currentnode = listhdr;
    bheader* nextnode = currentnode->next;
    if (nextnode == 0){
        currentnode->next = block;
        return true;
    }
    // shouldn't be a case, not inserted.
    if(currentnode > block){
        printf("ERROR: Node that is trying to be inserted before the list's first element. Not inserting.\n");
        return false;
    }
    while (nextnode != 0){
        // insertion in the middle
        if(currentnode < block && nextnode > block){
            currentnode->next = block;
            block->next = nextnode;
            return true;
        }
        currentnode = nextnode;
        nextnode = currentnode->next;
    }
    // hence, insert as the last element
    currentnode->next = block;
    return true;
}


void printbuf(char* buf){
    size_t len = strlen(buf);
    for(int i=0; i<len; i++){
        printf("%p : %c\n", &buf[i], buf[i]);
    }
}

void printbheader(bheader h){
    printf("***BLOCK HEADER***\n");
    for(int i=0; i<BHDRSIZE; i++){
        char* curbyte = (char*)(&h) + i;
        printf("%p : %d\n", curbyte, *curbyte);
    }
    printf("self at: %p\n", &(h.used));
    printf("used: %s\nsize: %zu\nnext header at: %p\n\n", h.used?"true":"false", h.size, &(h.next->used));
}

void printall(bheader start){
    bheader* next = &start;
    for(int i = 0; next!=0; i++){
        printf("*** HEADER %d ***\n", i);
        printbheader(*next);
        printbuf((char*)next + BHDRSIZE);
        next = next->next;
    }
}

/* returns a pointer to free memory where the value of size size can be written into */
void* malloc(size_t size){
    bheader* current = h.next;
    // pointer to next block
    bheader* previous = &(h);
    printf("Free list %s empty\n", current == 0? "is": "is not");
    printf("h.next : %p\n", current);
    printfull(h);
    while (current != 0){
        if(current->size >= size){
            printf("Encountered a free block large enough\n");
            // get the block off the free blocks list
            previous->next = previous->next->next;
            printf("previous: %p\n", previous);
            printf("free list next: %p\n", previous->next);
            current->used = true;
            if(!inserttoblocklist(current, &f)){
                printf("Failed to insert a block.\n");
                return 0;
            }
            printf("Block inserted\n");
            printf("Returning register %p\n", (char*)current + BHDRSIZE);
            return (char*)current + BHDRSIZE;
        }
        printf("Going to next free block\n");
        previous = current;
        current = current->next;
    }
    // if no free blocks
    // check if there is enough space left
    printf("Attemting to allocate new memory...\n");
    if(allocp + size < (char*)&(f) + ALLOCSIZE){
        // start with the pointer to the first block of the list
        current = f.next;
        // previous block
        previous = &(f);

        // check if it is the first block on the list
        // if it is, it has to take the free list header into account
        bool first = current == 0;
        printf("start block at: %p\n", previous);
        printf("next block at: %p\n", current);
        // as long as there's next, iterate
        printf(current == 0? "No occupied memory.\n": "Found occupied memory\n");
        while(current != 0){
            previous = current;
            current = current->next;
            printf("Going to next occupied memory block at %p\n", current);
        }
        // found last item - previous
        printf("Last header pointer: %p\n", previous);
        // take free list header into account
        if(first)
            previous->next = (bheader*)((char*)previous + previous->size + BHDRSIZE*2);
        else
            previous->next = (bheader*)((char*)previous + previous->size + BHDRSIZE);
        // set new block's header data
        current = previous->next;
        printf("New header pointer: %p\n", current);
        printf("New data pointer: %p\n", (char*)current + BHDRSIZE);
        printf("Data size: %zu\n", size);
        printf("Next available space: %p\n\n", (char*)current + BHDRSIZE + size);
        allocp = (char*)current + BHDRSIZE + size;
        current->size = size;
        current->used = true;
        current->next = 0;
        printf("Returning pointer: %p\n\n", (char*)current + BHDRSIZE);
        // where new block's data can be saved
        // lastblock's address + last block's size + size of last and new block's headers
        return (char*)current + BHDRSIZE;
    }else
        return 0;
    
}

void printheaders(bheader start){
    bheader* next = &start;
    for(int i = 0; next!=0; i++){
        printf("*** HEADER %d ***\n", i);
        printbheader(*next);
        next = next->next;
    }
}

// free a preexisting block of memory
void free(void* ptr){
    // get the pointer to header
    bheader* hdrp = (bheader*)(((char*)ptr) - BHDRSIZE);
    printf("Header pointer to free: %p\n", hdrp);
    // check if it is a valid header
    // iterate through full
    bheader* block = f.next;
    bheader* prev = &f;
    bool validhdr = false;
    while (block!=0 && !validhdr){
        printf("checking if %p matches %p.\n", block, hdrp);
        if(block == hdrp && block != &f && block->used){
            printf("valid header.\n");
            validhdr = true;
            // valid address
            // deleting it from the full adresses list
            prev->next = block->next;
            block->used = false;
            // insert into the free blocks list
            printf("h value before insertion: %p\n", &h);
            inserttoblocklist(block, &h);
            // printf("EMPTY\n");
            // printfull(h);
            memset(ptr, 0, block->size);
            return;
        }
        prev = block;
        block = block->next;
    }
    if(!validhdr){
        printf("Attempt to free block containing an address %p. It is not a valid address.\n", ptr);
        return;
    }
    
}

char* writestr(char string[]){
    printf("Inserting %s\n", string);
    printf("\n\n\n VALUE OF H: %p \n", &h);
    printf("\n VALUE OF H.NEXT: %p \n", h.next);
    printf("\n VALUE OF F: %p \n\n\n", &f);
    size_t size = strlen(string);
    // one extra for end of string
    char* p = malloc(size+1);
    printf("alloc start: %p\n", p);
    if(p != 0){
        int i;
        for(i = 0; i<size; i++){
            p[i] = string[i];
        }
        // end of string character
        p[i+1] = '\0';
        printfull(f);
        return p;
    }else
        printf("No more space left!\n");
        return 0;
}
