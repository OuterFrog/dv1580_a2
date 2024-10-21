#include "memory_manager.h"
#include "common_defs.h"
#include <stdbool.h>

void* memory;
size_t s;

memory_block* memory_blocks;
int block_count = 0;
int non_empty_blocks = 0;

// Initializes the memory manager, with a memory pool of size amount of bytes
void mem_init(size_t size){
    memory = malloc(size);
    s = size;

    memory_blocks = malloc(sizeof(memory_block) * s);

    memory_blocks[0] = (memory_block) {memory, s, true};
    block_count = 1;
}

void* mem_alloc(size_t size){
    //Walk through all memory blocks, trying to find a memory block that is large enough and free
    int i = 0;
    while(i < block_count && (!memory_blocks[i].free || memory_blocks[i].block_size < size)){
        i++;
    }

    //If it rejected all existing memory blocks, then allocation is impossible
    if(i >= block_count){
        printf_red("ERROR, no space in memory! \n");
        return NULL;
    }
    
    if(memory_blocks[i].block_size == size){
        // If the chosen memory block is the exact size to allocate, 
        // just mark it as allocated
        memory_blocks[i].free = false;
    }
    else{
        // Otherwise, change the size of the chosen memory block, and fill out the rest of
        // the space it previously occupied with a new empty memory block

        block_count++;
        
        // Create new memoryblock that starts at i.start + size
        void* new_start = (void*)((char*)memory_blocks[i].start + size);
        memory_blocks[block_count - 1] = (memory_block){new_start, memory_blocks[i].block_size - size, true};

        // Change old memory block size to allocated size
        memory_blocks[i].block_size = size;

        // Mark old memory block as allocated
        memory_blocks[i].free = false;
    }    

    non_empty_blocks++;

    return memory_blocks[i].start;
}

void mem_free(void* block){
    int i = 0;
    while(memory_blocks[i].start != block && i < block_count){
        i++;
    }

    // If it can't find the block, just return
    if(i >= block_count)
        return;

    // If the specified block was already free, just return
    if(memory_blocks[i].free)
        return;

    //Free the block
    memory_blocks[i].free = true;

    non_empty_blocks--;

    // Memory merging
    if(false && non_empty_blocks <= 0){
        // The easy case
        memory_blocks[0] = (memory_block) {memory, s, true};
        block_count = 1;
    }
    else{
        // And this is for every other case. 
        // Merging did not work well with the rest of my implementation, 
        // which is why it looks like this.
        // I also didn't know if we were allowed to add custom functions, 
        // which would make this more readable.
        int j = 0;
        // Loop through every memory block
        while(j < block_count - 1){
            if(memory_blocks[j].free){
                int l = 0;
                // If it is free, then loop through all memory blocks 
                // again to find the one directly after in memory.
                while(l < block_count){
                    if(memory_blocks[l].start == (void*)((char*)memory_blocks[j].start + memory_blocks[j].block_size)){
                        // If that one is also free, then merge them
                        if(memory_blocks[l].free){
                            memory_blocks[j].block_size += memory_blocks[l].block_size;

                            // To remove it from the array, loop through the rest of the array and shift everything one step.
                            for(int m = l; m < block_count - 1; m++){
                                memory_blocks[m] = memory_blocks[m + 1];
                            }

                            block_count--;
                            j--;
                            break;
                        }
                    }
                    
                    l++;
                }
            }
            j++;
        }
    }  
}

// Changes size of block
void* mem_resize(void* block, size_t size){
    // Delete old block
    mem_free(block);

    // Initialize new block
    void* new_block = mem_alloc(size);

    // Find index of memory block
    int i = 0;
    while(memory_blocks[i].start != block && i < block_count){
        i++;
    }

    // if memory_block doesn't exist, return NULL
    if(i >= block_count)
        return NULL;

    // Copy the data
    memcpy(new_block, block, memory_blocks[i].block_size);

    // Return the new block
    return new_block;
}

// Frees all memory that was allocated using malloc
void mem_deinit(){
    free(memory);
    free(memory_blocks);
}