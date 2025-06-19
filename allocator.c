#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>

#define MIN_PAYLOAD_SIZE 16

void* heap_start = NULL;
void* heap_end = NULL;
block_header* free_list = NULL;

typedef struct block_header{
    size_t payload_size;
    struct block_header* next;
    struct block_header* prev;
    unsigned int is_free;
    uint32_t padding;
} block_header;

void* init_allocate(size_t initial_size){
    heap_start = sbrk(0);
    if (heap_start == (void*)-1){
        return -1;
    }
    if (sbrk(initial_size) == (void*)-1){
        return -1;
    }
    heap_end = sbrk(0);
    if (heap_end == (void*)-1) {
        return -1;
    }
    free_list = (block_header*)heap_start;
    free_list->payload_size = initial_size - sizeof(block_header);
    free_list->is_free = 1;
    free_list->next = NULL;
    free_list->prev = NULL;
    
    return 0;
    
}

block_header* find_free_list(size_t required_size){
    block_header* current = free_list;
    while (current != NULL){
        if (current->is_free && current->payload_size >= required_size){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void* my_malloc(size_t size){
    if (size == 0){
        return NULL;
    } 
    size_t required_size = size + sizeof(block_header);
    block_header* new_block = NULL;

    //finding and setting a new memory block
    new_block = find_free_list(required_size);
    if (!new_block) return NULL;
    size_t total_available_size = new_block->payload_size;
    new_block->payload_size = required_size - sizeof(block_header);

    //calculating the leftover space to decide whether it is efficient to split or not
    size_t leftover_size = total_available_size - new_block->payload_size - sizeof(block_header);

    //splitting the memory space
    block_header* next_block = NULL;
    if (leftover_size >= MIN_PAYLOAD_SIZE){
        next_block = splitting(new_block, total_available_size);
    }
    new_block->is_free = 0;

    //changing the position of the free list
    if (next_block){
        //if next_block exists, it's a leftover block after splitting process
        next_block->prev = new_block->prev;
        next_block->next = new_block->next;

        if (new_block->prev){
            new_block->prev->next = next_block;
        }
        if (new_block->next){
            new_block->next->prev = next_block;
        }

        if (new_block == free_list){
            free_list = next_block;
        }
    }
    else{
        //no leftover block, we are removing new_block from the free_list entirely
        if (new_block->prev){
            new_block->prev->next = new_block->next;
        }
        if (new_block->next){
            new_block->next->prev = new_block->prev;
        }
        if (new_block == free_list){
            free_list = new_block->next;
        }
    }

    //returning the pointer to the available payload space
    void* payload_pointer = (char*)new_block + sizeof(block_header);
    return payload_pointer;
    
}

block_header* splitting(block_header* last_block, size_t total_size){
    if (!last_block) return NULL;

    char* split_point = (char*)last_block + sizeof(block_header) + last_block->payload_size;
    block_header* next_block = (block_header*)split_point;
    next_block->payload_size = total_size - last_block->payload_size - sizeof(block_header);
    next_block->is_free = 1;

    //insert the newly created block into the linked list of memory blocks
    next_block->prev = last_block;
    next_block->next = last_block->next;
    if (last_block->next){
        last_block->next->prev = next_block;
    }
    last_block->next = next_block;

    return next_block;

}

void my_free(void* payload_ptr){
    block_header* block = (block_header*)((char*)payload_ptr - sizeof(block_header));
    block->is_free = 1;

    //case 1: both adjacent memory blocks are free
    if (block->prev && block->next){
        if (block->prev->is_free == 1 && block->next->is_free == 1){
            block->prev->payload_size += (2 * sizeof(block_header) + block->payload_size + block->next->payload_size);
            block->prev->next = block->next->next;
            block->next->next->prev = block->prev;
        }
    }
    else if (block->prev || block->next){
        //case 2: only previous memory block is free
        if (block->prev->is_free == 1 && block->next->is_free == 0){
            block->prev->next = block->next;
            block->next->prev = block->prev;
            block->prev->payload_size += (sizeof(block_header) + block->payload_size);
        }
        //case 3: only next memory block is free
        if (block->prev->is_free == 0 && block->next->is_free == 1){
            block->next = block->next->next;
            block->next->prev = block;
            block->payload_size += (sizeof(block_header) + block->next->payload_size);
        }
    }
}

