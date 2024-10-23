#include "memory_manager.h"

bool should_debug = false;
#define DEBUG(x) if (should_debug) {x;}

pthread_mutex_t lock;

void* memory;
size_t s;

memory_block* memory_block_head;
int block_count = 0;

//bool ignore_locks = false;

// Initializes the memory manager, with a memory pool of size amount of bytes
void mem_init(size_t size){
    DEBUG(printf("mem_init: %d ", (int)size));

    pthread_mutexattr_t recursive_attr;
    pthread_mutexattr_init(&recursive_attr);
    pthread_mutexattr_settype(&recursive_attr, PTHREAD_MUTEX_RECURSIVE);
    pthread_mutex_init(&lock, NULL);

    memory = malloc(size);
    s = size;

    memory_block_head = malloc(sizeof(memory_block));

    *memory_block_head = (memory_block) {memory, s, true, NULL};
    block_count = 1;
}

void* mem_alloc(size_t size){
    pthread_mutex_lock(&lock);

    DEBUG(printf("mem_alloc: %d ", (int)size));
    //Walk through all memory blocks, trying to find a memory block that is large enough and free
    memory_block* walker = memory_block_head;
    while(walker != NULL && (!walker->free || walker->block_size < size)){
        walker = walker->next;
    }

    //If it rejected all existing memory blocks, then allocation is impossible
    if(walker == NULL){
        printf("ERROR, no space in memory! \n");

        //if(!ignore_locks)
            pthread_mutex_unlock(&lock);
        return NULL;
    }
    
    if(walker->block_size == size){
        // If the chosen memory block is the exact size to allocate, 
        // just mark it as allocated
        walker->free = false;
    }
    else{
        // Otherwise, change the size of the chosen memory block, and fill out the rest of
        // the space it previously occupied with a new empty memory block

        block_count++;
        
        // Create new memoryblock that starts at i.start + size
        void* new_start = (void*)((char*)walker->start + size);
        memory_block* new_block = malloc(sizeof(memory_block));
        *new_block = (memory_block){new_start, walker->block_size - size, true, walker->next};
        walker->next = new_block;

        // Change old memory block size to allocated size
        walker->block_size = size;

        // Mark old memory block as allocated
        walker->free = false;
    }    

    DEBUG(printf("at %d ", (int)walker->start));
    //if(!ignore_locks)
        pthread_mutex_unlock(&lock);
    return walker->start;
}

void mem_free(void* block){
    pthread_mutex_lock(&lock);
    DEBUG(printf("memfree: %d ", (int)block));

    memory_block* block_to_free = memory_block_head;
    memory_block* block_preceding = NULL;

    if(block == NULL){
        pthread_mutex_unlock(&lock);
        return;
    }

    if(block_to_free->start != block){
        memory_block* walker = memory_block_head;
        while(walker != NULL && walker->next != NULL && ((memory_block*)walker->next)->start != block) {
            walker = walker->next;
        }

        // If it can't find the block, just return
        if(walker == NULL || walker->next == NULL){
            printf("ERROR, no such block to free! \n");
            //if(!ignore_locks)
                pthread_mutex_unlock(&lock);
            return;
        }

        block_to_free = walker->next;
        block_preceding = walker;
    }

    // If the specified block was already free, just return
    if(block_to_free->free){
        //if(!ignore_locks)
            pthread_mutex_unlock(&lock);
        return;
    }


    //Free the block
    block_to_free->free = true;

    // Memory merging

    // Merge with previous block
    if(block_preceding != NULL && block_preceding->free){
        block_preceding->next = block_to_free->next;
        block_preceding->block_size += block_to_free->block_size;
        block_count--;
        free(block_to_free);
        block_to_free = block_preceding;
    }

    // Merge with next block
    memory_block* next_block = ((memory_block*)block_to_free->next);
    if(next_block != NULL && next_block->free){
        block_to_free->next = next_block->next;
        block_to_free->block_size += next_block->block_size;
        free(next_block);
        block_count--;
    }

    //if(!ignore_locks)
        pthread_mutex_unlock(&lock);
}

// Changes size of block
void* mem_resize(void* block, size_t size){
    pthread_mutex_lock(&lock);
    DEBUG(printf("mem_resize: %d ", (int)size));

    memory_block* block_to_resize = memory_block_head;
    memory_block* block_preceding = NULL;

    if(block_to_resize->start != block){
        memory_block* walker = memory_block_head;
        while(walker != NULL && walker->next != NULL && ((memory_block*)walker->next)->start != block) {
            walker = walker->next;
        }

        // If it can't find the block, just return
        if(walker == NULL || walker->next == NULL){
            printf("ERROR, no such block to free! \n");
            pthread_mutex_unlock(&lock);
            return NULL;
        }

        block_to_resize = walker->next;
        block_preceding = walker;
    }

    // Resizing forward
    memory_block* block_after = block_to_resize->next;
    if (block_after != NULL && block_after->free && block_after->block_size + block_to_resize->block_size >= size){
        block_to_resize->next = block_after->next;
        block_to_resize->block_size += block_after->block_size;
        block_count--;
        free(block_after);

        DEBUG(printf("Resized forward "));

        pthread_mutex_unlock(&lock);
        return block_to_resize->start;
    }
    // Resizing backward
    else if(block_preceding != NULL && block_preceding->free && block_preceding->block_size + block_to_resize->block_size >= size){
        block_preceding->next = block_to_resize->next;
        block_preceding->block_size += block_to_resize->block_size;
        block_count--;
        free(block_to_resize);

        DEBUG(printf("Resized backward "));

        pthread_mutex_unlock(&lock);
        return block_preceding->start;
    }
    else{
        // Allocate new block
        //ignore_locks = true;
        void* new_block = mem_alloc(size);
        if(new_block != NULL){
            // Copy the data
            memcpy(new_block, block, block_to_resize->block_size);

            // Delete old block
            mem_free(block);


            //ignore_locks = false;

            // Return the new block
            pthread_mutex_unlock(&lock);
            return new_block;           
        } 
        else{
            printf("ERROR: No space for resized block!");
            pthread_mutex_unlock(&lock);
            return NULL;
        }
    }
}

// Frees all memory that was allocated using malloc
void mem_deinit(){
    DEBUG(printf("mem_deinit "));

    free(memory);
    
    memory_block* walker_of_death = memory_block_head;
    while(walker_of_death != NULL) {
        memory_block* to_del = walker_of_death;
        walker_of_death = walker_of_death->next;
        free(to_del);
    }

    pthread_mutex_destroy(&lock);
}