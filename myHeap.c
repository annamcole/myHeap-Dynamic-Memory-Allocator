///////////////////////////////////////////////////////////////////////////////
//
// Copyright 2019-2020 Jim Skrentny
// Posting or sharing this file is prohibited, including any changes/additions.
// Used by permission Fall 2020, CS354-deppeler
//
///////////////////////////////////////////////////////////////////////////////
 
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include "myHeap.h"
 
/*
 * This structure serves as the header for each allocated and free block.
 * It also serves as the footer for each free block but only containing size.
 */
typedef struct blockHeader {           
    int size_status;
    /*
    * Size of the block is always a multiple of 8.
    * Size is stored in all block headers and free block footers.
    *
    * Status is stored only in headers using the two least significant bits.
    *   Bit0 => least significant bit, last bit
    *   Bit0 == 0 => free block
    *   Bit0 == 1 => allocated block
    *
    *   Bit1 => second last bit 
    *   Bit1 == 0 => previous block is free
    *   Bit1 == 1 => previous block is allocated
    * 
    * End Mark: 
    *  The end of the available memory is indicated using a size_status of 1.
    * 
    * Examples:
    * 
    * 1. Allocated block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 27
    *      If the previous block is free, size_status should be 25
    * 
    * 2. Free block of size 24 bytes:
    *    Header:
    *      If the previous block is allocated, size_status should be 26
    *      If the previous block is free, size_status should be 24
    *    Footer:
    *      size_status should be 24
    */
} blockHeader;         

/* Global variable - DO NOT CHANGE. It should always point to the first block,
 * i.e., the block at the lowest address.
 */
blockHeader *heapStart = NULL;     

/* Size of heap allocation padded to round to nearest page size.
 */
int allocsize;

/*
 * Additional global variables may be added as needed below
 */
blockHeader *currentBlock = NULL;
 
/* 
 * Function for allocating 'size' bytes of heap memory.
 * Argument size: requested size for the payload
 * Returns address of allocated block on success.
 * Returns NULL on failure.
 * This function should:
 * - Check size - Return NULL if not positive or if larger than heap space.
 * - Determine block size rounding up to a multiple of 8 and possibly adding padding as a result.
 * - Use NEXT-FIT PLACEMENT POLICY to chose a free block
 * - Use SPLITTING to divide the chosen free block into two if it is too large.
 * - Update header(s) and footer as needed.
 * Tips: Be careful with pointer arithmetic and scale factors.
 */
void* myAlloc(int size) {
    if(currentBlock == NULL)
        currentBlock = heapStart;
        
    // return NULL if size is not positive
    if(size <= 0)
        return NULL;
        
    // return null if size is larger than heap space
    if(size > allocsize)
        return NULL;
    
    int t_size;
    blockHeader *current = currentBlock;
    
    // Determine to-be-allocated block size
    int blockSize = -1;
    int preSize = sizeof(blockHeader) + size;
    int nPadding = preSize % 8;
    if(nPadding != 0)
        blockSize = preSize + (8 - nPadding);
    else
        blockSize = preSize;
        
    int prev_used = -1;
    int curr_used = -1;
    
    int firstBlock = 1; // firstBlock == 1 when current equals the current block in first check
    
    // Use NEXT-FIT placement to choose free block
    while(firstBlock == 1 || current != currentBlock) {
        t_size = current->size_status;
        
        // default
        prev_used = -1;
        curr_used = -1;
        
        //check if current block is end of heap
        if(t_size == 1) {
            current = (blockHeader*)((void*)heapStart);
            firstBlock = -1;
            continue;
        }
        
        // analyze block flags
        if((t_size & 1) != 0) {
            curr_used = 1;
            t_size = t_size - 1;            
        }
        if((t_size & 2) != 0) {
            prev_used = 1;
            t_size = t_size - 2;
        }
        
        // check if current block is allocated
        if(curr_used == 1) {
            current = (blockHeader*)((void*)current + t_size);
            firstBlock = -1;
            continue;
        }
        
        // check if current block has required size
        if(t_size < blockSize) {
            current = (blockHeader*)((void*)current + t_size);
            firstBlock = -1;
            continue;
        }
        else
          break;
        
    }
    
    // full check of heap has occurred without finding a block that fits
    if(current == currentBlock && firstBlock != 1)
        return NULL;
    
    // check if selected block can be split to reduce internal fragmentation
    blockHeader *next = NULL;
    blockHeader *next_footer = NULL;
    blockHeader *current_footer = NULL;
    
    int new_block_size = 0;
    int split_size = t_size - blockSize;
    
    if(split_size >= 16) 
    {
        // split memory to reduce internal fragmentation 
        
        // set up next header
        next = (blockHeader*)((void*)current + blockSize);
        next->size_status = split_size + 2; // set p bit high
        // set up next footer
        next_footer = (blockHeader*)((void*)next + split_size - 4);
        next_footer->size_status = split_size;
        
        
        // set up current block
        if(prev_used) 
            new_block_size += 2 + 1; // 2 sets previous used flag, 1 sets current used flag
        else
            new_block_size += 1; // 1 sets current used flag
               
        current->size_status = blockSize + new_block_size;   
        // set up current footer
        current_footer = (blockHeader*)((void*)current + blockSize - 4);
        current_footer->size_status = blockSize;
        
        // update t_size
        t_size = blockSize;
             
    } else {
        // set up block
        if(prev_used)
            new_block_size += 2 + 1;  // set p and a bits
        else
            new_block_size += 1;  // set a bit
        
        current->size_status = t_size + new_block_size;
        
        next = (blockHeader*)((void*)current + t_size);
        next->size_status += 2;
    }
    
    // change next block's prev flag to used & update currentBlock
    currentBlock = (blockHeader*)((void*)current + t_size);
    blockHeader *ptr = (blockHeader*)((void*)current + sizeof(blockHeader));
    
    return ptr;
} 
 
/* 
 * Function for freeing up a previously allocated block.
 * Argument ptr: address of the block to be freed up.
 * Returns 0 on success.
 * Returns -1 on failure.
 * This function should:
 * - Return -1 if ptr is NULL.
 * - Return -1 if ptr is not a multiple of 8.
 * - Return -1 if ptr is outside of the heap space.
 * - Return -1 if ptr block is already freed.
 * - USE IMMEDIATE COALESCING if one or both of the adjacent neighbors are free.
 * - Update header(s) and footer as needed.
 */                    
int myFree(void *ptr) {    
    //TODO: Your code goes in here.
    // check if pointer is NULL
    if(ptr == NULL)
        return -1;
    
    // check if pointer is a multiple of 8
    if((int)ptr % 8 != 0)
        return -1;
        
    int t_size; 
    blockHeader *current = (blockHeader*)((void*)ptr - 4);
    blockHeader *currFooter = NULL;
    // get endmark address
    blockHeader *heapEnd = (blockHeader*)((void*)heapStart + allocsize);
    blockHeader *prevHeader = NULL;
    blockHeader *prevFooter = NULL;
    blockHeader *nextHeader = NULL;
    blockHeader *nextFooter = NULL;
    
    // check if ptr is out of heap bounds
    if(ptr < (void*)heapStart || ptr > (void*)heapEnd)  // TODO: might need to fix casting
        return -1;
    
    int prev_used = 0;
    int curr_used = 0;
    int next_used = 0;
    
    int prevSize = 0;
    t_size = current->size_status;
    int nextSize = 0;
    
    // analyze current block
    if((t_size & 1) != 0) {
        curr_used = 1;
        t_size = t_size - 1;            
    }
    if((t_size & 2) != 0) {
        prev_used = 1;
        t_size = t_size - 2;
    }
    currFooter = (blockHeader*)((void*)current + t_size - 4);
    
    // check if current block is allocated
    if(curr_used == 0)
        return -1;
      
    // analyze previous block
    prevFooter = (blockHeader*)((void*)current - 4);
    prevSize = prevFooter->size_status;
    prevHeader = (blockHeader*)((void*)current - prevSize);
    prevSize = prevHeader->size_status; // update to include p and a bits
    if((prevSize & 1) != 0) {
        prev_used = 1;
        prevSize = prevSize - 1;            
    }
    if((prevSize & 2) != 0) {
        prevSize = prevSize - 2;
    }
    
    // analayze next block
    nextHeader = (blockHeader*)((void*)current + t_size);
    nextSize = nextHeader->size_status;
    if((nextSize & 1) != 0) {
        next_used = 1;
        nextSize = nextSize - 1;            
    }
    if((nextSize & 2) != 0) {
        nextSize = nextSize - 2;
    }
    nextFooter = (blockHeader*)((void*)nextHeader + nextSize - 4);
    
    // set current block a-bit and next block p-bit to free
    curr_used = 0;
    current->size_status -= 1;
    nextHeader->size_status -= 2;
    
    
    // coalesce free blocks
    
    // check if previous block is free
    if(prev_used == 0) {     
          
        // update coalesced block size, p bit should already be set
        prevHeader->size_status += t_size;
        
        // update footer value
        currFooter->size_status = prevSize + t_size;
        
        // update global current block pointer
        currentBlock = prevHeader;
        
    } else
        currentBlock = current;
    
    // check if next block is free
    if(next_used == 0) {
        // update coalesced block size, p bit should already be set
        currentBlock->size_status += nextSize;
        
        // update footer value
        if(current == heapStart)
          nextFooter->size_status = prevSize + nextSize;
        else
          nextFooter->size_status = prevSize + t_size + nextSize;
    }
    
    return 0;
} 
 
/*
 * Function used to initialize the memory allocator.
 * Intended to be called ONLY once by a program.
 * Argument sizeOfRegion: the size of the heap space to be allocated.
 * Returns 0 on success.
 * Returns -1 on failure.
 */                    
int myInit(int sizeOfRegion) {    
 
    static int allocated_once = 0; //prevent multiple myInit calls
 
    int pagesize;  // page size
    int padsize;   // size of padding when heap size not a multiple of page size
    void* mmap_ptr; // pointer to memory mapped area
    int fd;

    blockHeader* endMark;
  
    if (0 != allocated_once) {
        fprintf(stderr, 
        "Error:mem.c: InitHeap has allocated space during a previous call\n");
        return -1;
    }
    if (sizeOfRegion <= 0) {
        fprintf(stderr, "Error:mem.c: Requested block size is not positive\n");
        return -1;
    }

    // Get the pagesize
    pagesize = getpagesize();

    // Calculate padsize as the padding required to round up sizeOfRegion 
    // to a multiple of pagesize
    padsize = sizeOfRegion % pagesize;
    padsize = (pagesize - padsize) % pagesize;

    allocsize = sizeOfRegion + padsize;

    // Using mmap to allocate memory
    fd = open("/dev/zero", O_RDWR);
    if (-1 == fd) {
        fprintf(stderr, "Error:mem.c: Cannot open /dev/zero\n");
        return -1;
    }
    mmap_ptr = mmap(NULL, allocsize, PROT_READ | PROT_WRITE, MAP_PRIVATE, fd, 0);
    if (MAP_FAILED == mmap_ptr) {
        fprintf(stderr, "Error:mem.c: mmap cannot allocate space\n");
        allocated_once = 0;
        return -1;
    }
  
    allocated_once = 1;

    // for double word alignment and end mark
    allocsize -= 8;

    // Initially there is only one big free block in the heap.
    // Skip first 4 bytes for double word alignment requirement.
    heapStart = (blockHeader*) mmap_ptr + 1;

    // Set the end mark
    endMark = (blockHeader*)((void*)heapStart + allocsize);
    endMark->size_status = 1;

    // Set size in header
    heapStart->size_status = allocsize;

    // Set p-bit as allocated in header
    // note a-bit left at 0 for free
    heapStart->size_status += 2;

    // Set the footer
    blockHeader *footer = (blockHeader*) ((void*)heapStart + allocsize - 4);
    footer->size_status = allocsize;
  
    return 0;
} 
                  
/* 
 * Function to be used for DEBUGGING to help you visualize your heap structure.
 * Prints out a list of all the blocks including this information:
 * No.      : serial number of the block 
 * Status   : free/used (allocated)
 * Prev     : status of previous block free/used (allocated)
 * t_Begin  : address of the first byte in the block (where the header starts) 
 * t_End    : address of the last byte in the block 
 * t_Size   : size of the block as stored in the block header
 */                     
void dispMem() {     
 
    int counter;
    char status[5];
    char p_status[5];
    char *t_begin = NULL;
    char *t_end   = NULL;
    int t_size;

    blockHeader *current = heapStart;
    blockHeader *footer = NULL;
    counter = 1;

    int used_size = 0;
    int free_size = 0;
    int is_used   = -1;
    int footerSize = -1;

    fprintf(stdout, "************************************Block list***\
                    ********************************\n");
    fprintf(stdout, "No.\tStatus\tPrev\tt_Begin\t\tt_End\t\tt_Size\t\tFooterSize\n");
    fprintf(stdout, "-------------------------------------------------\
                    --------------------------------\n");
  
    while (current->size_status != 1) {
        t_begin = (char*)current;
        t_size = current->size_status;
    
        if (t_size & 1) {
            // LSB = 1 => used block
            strcpy(status, "used");
            is_used = 1;
            t_size = t_size - 1;
        } else {
            strcpy(status, "Free");
            is_used = 0;
        }

        if (t_size & 2) {
            strcpy(p_status, "used");
            t_size = t_size - 2;
        } else {
            strcpy(p_status, "Free");
        }

        if (is_used) 
            used_size += t_size;
        else 
            free_size += t_size;

        t_end = t_begin + t_size - 1;
        footer = (blockHeader*)((void*)current + t_size - 4);
        footerSize = footer->size_status;
    
        fprintf(stdout, "%d\t%s\t%s\t0x%08lx\t0x%08lx\t%d\t\t%d\n", counter, status, 
        p_status, (unsigned long int)t_begin, (unsigned long int)t_end, t_size,footerSize);
    
        current = (blockHeader*)((char*)current + t_size);
        counter = counter + 1;
    }

    fprintf(stdout, "---------------------------------------------------\
                    ------------------------------\n");
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fprintf(stdout, "Total used size = %d\n", used_size);
    fprintf(stdout, "Total free size = %d\n", free_size);
    fprintf(stdout, "Total size = %d\n", used_size + free_size);
    fprintf(stdout, "***************************************************\
                    ******************************\n");
    fflush(stdout);

    return;  
} 


// end of myHeap.c (fall 2020)

