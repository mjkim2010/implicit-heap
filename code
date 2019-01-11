#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "allocator.h"
#include "debug_break.h"
#define HEADER_SIZE 4

struct block {
    int size;
    void* ptr;
};

void *bestFit(size_t requestedsz);
struct block compareBlocks(struct block currBlock, struct block bestBlock, size_t requestedsz);
void *findHeader(void *payload);
    
static void *seg_start;
static size_t seg_size, nused;
static size_t freeBlock, inuse;


//Executes a bitwise round up to the nearest multiple 
size_t roundup(size_t sz, size_t mult)
{
    return (sz + mult-1) & ~(mult-1);
}

//initialize global variables based on segment boundaries
bool myinit(void *segment_start, size_t segment_size)
{
    seg_start = segment_start;
    seg_size = segment_size;
    nused = 0;
    freeBlock = 0;
    inuse = 0;
    return true;
}

//Allocates a block of requestedsz and returns a ptr to the payload of the block.
void *mymalloc(size_t requestedsz)
{
    if (requestedsz == 0 || requestedsz > 1<<30) {
        return NULL;
    }
    if (nused == 0) {
        nused = HEADER_SIZE;
    }
    void *ptr = bestFit(requestedsz);
    if (ptr == NULL) {   //no current block can fit request
        ptr = (char*)seg_start + nused + HEADER_SIZE;
        size_t needed = roundup(nused+requestedsz+HEADER_SIZE, HEADER_SIZE); 
        if (roundup(needed, ALIGNMENT) == needed) {   //make sure needed isn't multiple of 8
            needed += HEADER_SIZE;
        }
        int header = needed-nused;
        header |= 0x1;   //sets first bit to 1, indicates used
        *((int*)seg_start+nused/HEADER_SIZE) = header;
        nused = needed;
    }
    inuse++;
    return ptr;
}

// Searches for the "best fit" block amongst all the free blocks by iterating through
// the entire USED heap and updating the best block to be the one with the size closest
// to the requestedsz. Returns a ptr to the payload of said block. 
void *bestFit(size_t requestedsz) {
    int header = HEADER_SIZE;
    struct block bestBlock;
    bestBlock.ptr = NULL;    
    bestBlock.size = ~(1<<31); //want to return size of -1 in case no block can fit requestedsz
    while (header < nused) {    //traverses all current blocks
        int bytes = 0;
        bytes = *((int*)seg_start+header/HEADER_SIZE);
        if ((bytes & 1) == 0) {   //if "free"
            struct block currBlock;
            currBlock.ptr = (char*)seg_start + header + HEADER_SIZE;
            currBlock.size = bytes;
            bestBlock = compareBlocks(currBlock, bestBlock, requestedsz);
        } else {
            bytes ^= 1;
        }
        header += bytes;
    } 
    if (bestBlock.ptr != NULL) {
        *(int*)findHeader(bestBlock.ptr) = *(int*)findHeader(bestBlock.ptr)+1;
        freeBlock--;
        return bestBlock.ptr;
    }
    return NULL;
}

//Compares two free blocks and updates bestBlock to reflect the block that is closer
//in size to requestedsz.
struct block compareBlocks(struct block currBlock, struct block bestBlock, size_t requestedsz) {
    int currDiff = currBlock.size - requestedsz - HEADER_SIZE;
    int bestDiff = bestBlock.size - requestedsz - HEADER_SIZE;
    if (currDiff < 0) {
        return bestBlock;   
    } else if (currDiff == 0 || currDiff < bestDiff) {   //perfect fit or better fit than current best 
        return currBlock;
    } else {
        return bestBlock;
    }
}

//Frees the ptr
void myfree(void *ptr)
{
    if (ptr != NULL) {
        *(int*)findHeader(ptr) = *(int*)findHeader(ptr)-1;
        freeBlock++;
        inuse--;
    }
}

//Reallocates a pointer to a new block of size newsz. Does not implement in-place realloc
void *myrealloc(void *oldptr, size_t newsz)
{
    if (oldptr != NULL && newsz == 0) {
        myfree(oldptr);
        return NULL;
    } 
    void *newptr = mymalloc(newsz);
    if (oldptr != NULL) {
        memcpy(newptr, oldptr, newsz);
        myfree(oldptr);
    }
    return newptr;

}

//Returns the value stored in the header of a block. Reflects the size of header+payload only.
void *findHeader(void *payload) {
    return (int*)payload-1;
}
