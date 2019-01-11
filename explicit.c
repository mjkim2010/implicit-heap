#include "allocator.h"
#include "debug_break.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define HEADER_SIZE 4
#define MIN_BLOCK_SIZE 16

//Stores the size of a block (header+payload) and a ptr to its payload
struct block {
    int size;
    void* ptr;
};

void *findHeader(void *payload);
void *traverseFree(size_t requestedsz);
void *findBestFit(size_t requestedsz, void *currFree);
struct block cmpBlocks(struct block currBlock, struct block bestBlock, size_t requestedsz);
void updateFreeMalloc(void *bestFit);
bool coalesce(void *ptr, int currBlock);
void updateFreeCoalesce(void *nextBlock, void *currBlock);
void *returnPtr(void *curr);
void addLink(void *ptr);
void setPtr(void *curr, void *other);
int calcHeader(void *oldptr, size_t newsz);
void splitBlock(int initSize, int leftHeader, void *leftPtr); 
int setHeader(size_t needed, size_t used);

static void *seg_start;
static size_t seg_size, nused;
static int freeBlock, inuse;
static void *free_list;

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
    free_list = NULL;
    freeBlock = 0;
    inuse = 0;
    return true;
}

//Allocates a block of size requestedsz. Returns a pointer to the payload of the block.
void *mymalloc(size_t requestedsz)
{
    if (requestedsz == 0 || requestedsz > 1<<30) {
        return NULL;
    }
    if (nused == 0) {
        nused = HEADER_SIZE;
    }
    void *ptr = traverseFree(requestedsz);
    if (ptr == NULL) {
        ptr = (char*)seg_start + nused + HEADER_SIZE;
        int buffer = 0;
        if (MIN_BLOCK_SIZE - (int)requestedsz > 0) {
            buffer += MIN_BLOCK_SIZE - requestedsz;
        }
        size_t needed = roundup(nused+HEADER_SIZE+requestedsz+buffer, HEADER_SIZE);
        nused = setHeader(needed, nused);
    }
    inuse++;
    return ptr;
}

//Traverses the free list. Implements the best fit algorithm to find the free block that
//is closest in size to requestedsz. Returns a ptr to the "best fit" block. Return NULL if no block
//with minimum size requestedsz is found.
void *traverseFree(size_t requestedsz) 
{
    if (free_list != NULL) {
        void *currFree = free_list;   //points to header
        void *bestFit = findBestFit(requestedsz, currFree);
        if (bestFit != NULL) {
            updateFreeMalloc(bestFit);
            bestFit = (char*)bestFit+HEADER_SIZE;   //now points to payload
            *(int*)findHeader(bestFit) = *(int*)findHeader(bestFit)+1;
            freeBlock--;
            return bestFit;
        }
    } 
    return NULL;
}

//Returns a struct with the ptr pointing to the HEADER of the best fit block. 
void *findBestFit(size_t requestedsz, void *currFree) 
{
    struct block bestBlock;
    bestBlock.ptr = NULL;  
    bestBlock.size = ~(1<<31);
    while (currFree != NULL) {
        struct block currBlock;
        currBlock.ptr = currFree;
        currBlock.size = *(int*)currFree;
        bestBlock = cmpBlocks(currBlock, bestBlock, requestedsz);
        currFree = returnPtr((char*)currFree+HEADER_SIZE);
    }
    return bestBlock.ptr;
}

//Compares two free blocks and updates bestBlock to reflect the block that is closer in
//size to requestedsz. Returns a struct containing a ptr to the payload of the best fit block
//and the size of the block.
struct block cmpBlocks(struct block currBlock, struct block bestBlock, size_t requestedsz) 
{
    int currDiff = currBlock.size - requestedsz - HEADER_SIZE;
    int bestDiff = bestBlock.size - requestedsz - HEADER_SIZE;
    if (currDiff < 0) {
        return bestBlock;
    } else if (currDiff == 0 || currDiff < bestDiff) {
        return currBlock;
    } else {
        return bestBlock;
    }
}

//Updates the free list in the scenario where a once free block is about to be 
//allocated
void updateFreeMalloc(void *bestFit) 
{
    char *nextPtr = returnPtr((char*)bestFit+HEADER_SIZE);
    char *prevPtr = returnPtr((char*)bestFit+HEADER_SIZE+sizeof(char*));

    if (nextPtr == NULL && prevPtr == NULL) {   //only free block
        free_list = NULL;
    } else if (prevPtr == NULL) {    //first block in free_list
        free_list = nextPtr;
        setPtr(nextPtr+HEADER_SIZE+sizeof(char*), NULL);
    } else if (nextPtr == NULL) {   //last block in free_list
        setPtr(prevPtr+HEADER_SIZE, NULL);
    } else {    //in middle of two free blocks
        setPtr(prevPtr+HEADER_SIZE, nextPtr);
        setPtr(nextPtr+HEADER_SIZE+sizeof(char*), prevPtr);
    }
}

//Sets the header of the payload with the correct payload size + header size. Returns the difference in bytes
//from the start of the heap to the END of the payload
int setHeader(size_t needed, size_t used) 
{
    if (roundup(needed, ALIGNMENT) == needed) {   //make sure needed isn't multiple of 8
        needed += HEADER_SIZE;
    }
    int header = needed-used;
    header |= 0x1;   //sets first bit to 1, indicates used
    *((int*)seg_start+used/HEADER_SIZE) = header; 
    return needed;
}

//Frees the ptr
void myfree(void *ptr)
{
    if (ptr != NULL) {
        int currBlock = *(int*)findHeader(ptr)-1;  //size of payload+header
        if (!coalesce(ptr, currBlock)) {
            addLink(ptr);
        }             
        inuse--;
        freeBlock++;
    }
}

//Checks if the nextBlock to the currBlock (one being freed) is also free. If so,
//then currBlock will have a header that reflects the size of nextBlock as well as 
//itself so that both blocks appear as one larger block. 
bool coalesce(void *ptr, int currBlock) 
{
    void *nextBlock = (char*)ptr + currBlock;   //payload, not header
    int nextSize = *(int*)findHeader(nextBlock);   //header of nextBlock
    if ((nextSize & 1) == 0 && (nextSize!= 0)) {    //if free
        int newSize = currBlock + nextSize;
        *(int*)findHeader(ptr) = newSize;
        updateFreeCoalesce(nextBlock, ptr);
        freeBlock--;
        return true;
    } else {
        *(int*)findHeader(ptr) = currBlock;   //no coalescing
        return false;
    }
}

//Updates the free list in the scenario where a newly freed block is just about
//to be coalesced with its neighboring block, which has already been freed before.
void updateFreeCoalesce(void *nextBlock, void *currBlock) 
{
    char *postBlock = returnPtr(nextBlock);    //ptr to free block after nextBlock, points to header
    char *preBlock = returnPtr((char*)nextBlock+sizeof(char*));    //ptr to free block before nextBlock, points to header

    if (postBlock == NULL && preBlock == NULL) {    //coalesced block is first in free list
        free_list = findHeader(currBlock);
        setPtr(currBlock, NULL);
        setPtr((char*)currBlock+sizeof(char*), NULL);
    } else if (postBlock == NULL) {    //nextBlock is the last in free list
        setPtr(preBlock+HEADER_SIZE, NULL);
        addLink(currBlock);
    } else if (preBlock == NULL) {
        free_list = findHeader(currBlock);
        setPtr((char*)currBlock+sizeof(char*), NULL);
        setPtr(currBlock, postBlock);
        setPtr(postBlock+HEADER_SIZE+sizeof(char*), free_list);
    } else {
        setPtr(preBlock+HEADER_SIZE, postBlock);
        setPtr(postBlock+HEADER_SIZE+sizeof(char*), preBlock);
        addLink(currBlock);
    }
}

//Returns the ptr stored at the address indicated by curr. Will either be the nextptr or the prevptr of a payload in a freed block. 
void *returnPtr(void *curr) 
{
    void **currptr = curr;
    return *(void**)currptr;
}

//Adds the most recently freed block, indicated by ptr, to the start
//of free_list. Updates the nextptr within current block to point to the
//header of the former first block in free_list.
void addLink(void *ptr) 
{
    void *temp = free_list;  //formerly the first block in free_list
    free_list = findHeader(ptr);
    if (temp == NULL) {    //first block to ever be in free_list
        setPtr((char*)free_list+HEADER_SIZE, NULL);
    } else {
        setPtr((char*)free_list+HEADER_SIZE, temp);
        setPtr((char*)temp+HEADER_SIZE+sizeof(char*), free_list);   //update prevpointer of what was formerly first in free_list
    }
    setPtr((char*)free_list+HEADER_SIZE+sizeof(char*), NULL);   //sets prevptr of first in free_list to NULL
}

//Sets a pointer curr to point to other. Curr will either be the prevptr or nextptr 
//within the first 16 bytes of a payload.
void setPtr(void *curr, void *other) 
{
    void **nextptr = curr;
    *(void**)nextptr = other;
}

//Reallocates a pointer to size newsz and returns an address to the newly reallocated payload.
void *myrealloc(void *oldptr, size_t newsz)
{
    if (oldptr != NULL && newsz == 0) {
        myfree(oldptr);
        return NULL;
    }  
    void *newptr;
    if (oldptr != NULL) {
        int oldSize = *(int*)findHeader(oldptr)-HEADER_SIZE-1;    //payload size of oldptr       
        if (oldSize == roundup(newsz, HEADER_SIZE)) { 
            return oldptr;
        } else if (oldSize > roundup(newsz, HEADER_SIZE)) {
            int header = calcHeader(oldptr, newsz);   //header of newly realloc'd block
            splitBlock(oldSize+HEADER_SIZE, header, findHeader(oldptr));
            return oldptr;
        } else {
            newptr = mymalloc(newsz);
            memcpy(newptr, oldptr, newsz);
            myfree(oldptr);
        }
    } else {
       newptr = mymalloc(newsz);
    }
    return newptr;
} 

//Returns header for newly reallocated block. 
int calcHeader(void *oldptr, size_t newsz) 
{
    int used = (char*)findHeader(oldptr)-(char*)seg_start;
    int needed = roundup(used+HEADER_SIZE+newsz, HEADER_SIZE);
    return setHeader(needed,used) - used;
}


//initSize = size of entire block before it got split
//leftHeader = header of front of block
//leftPtr: points to header of front of block
//
//Splits a block if the end of the block is larger than the minimum size,
//which accounts for the header and the two pointers in the payload
void splitBlock(int initSize, int leftHeader, void *leftPtr) 
{
    int newHeader = initSize-leftHeader;   //header of block to be split off
    if (newHeader != 0) {
        void *newBlock = (char*)leftPtr + leftHeader + HEADER_SIZE;   //points to payload
        if (newHeader < MIN_BLOCK_SIZE+HEADER_SIZE) {
            if (!coalesce(newBlock, newHeader)) {
                *(int*)leftPtr = leftHeader+newHeader+1;
            } else {
                freeBlock++;
            }
        } else {
            *(int*)findHeader(newBlock) = newHeader+1;
            inuse++;
            myfree(newBlock);
        }
    }
}

//Returns a pointer to the header, given a pointer to the corresponding payload 
void *findHeader(void *payload) {
    return (int*)payload-1;
}
