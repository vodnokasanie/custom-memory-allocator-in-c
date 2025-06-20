#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdio.h>
#include <assert.h>
#include "allocator.h"

#define MIN_PAYLOAD_SIZE 16
#define DEFAULT_HEAP_SIZE 4096
#define ALIGNMENT 8

// Global variables
void* heap_start = NULL;
void* heap_end = NULL;

typedef struct block_header {
    size_t payload_size;
    struct block_header* next;
    struct block_header* prev;
    unsigned int is_free;
    uint32_t magic;  // For debugging and corruption detection
} block_header;

block_header* free_list = NULL;

#define MAGIC_FREE 0xDEADBEEF
#define MAGIC_ALLOCATED 0xFEEDFACE

// Function declarations
block_header* find_free_block(size_t required_size);
block_header* split_block(block_header* block, size_t required_size);
void coalesce_block(block_header* block);
void add_to_free_list(block_header* block);
void remove_from_free_list(block_header* block);
void* expand_heap(size_t size);
size_t align_size(size_t size);

// Align size to ALIGNMENT boundary
size_t align_size(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

// Initialize the heap allocator
void* init_allocator(size_t initial_size) {
    if (heap_start != NULL) {
        return heap_start; // Already initialized
    }
    
    if (initial_size < sizeof(block_header) + MIN_PAYLOAD_SIZE) {
        initial_size = DEFAULT_HEAP_SIZE;
    }
    
    initial_size = align_size(initial_size);
    
    heap_start = sbrk(0);
    if (heap_start == (void*)-1) {
        return NULL;
    }
    
    if (sbrk(initial_size) == (void*)-1) {
        heap_start = NULL;
        return NULL;
    }
    
    heap_end = sbrk(0);
    if (heap_end == (void*)-1) {
        heap_start = NULL;
        return NULL;
    }
    
    // Initialize the first free block
    free_list = (block_header*)heap_start;
    free_list->payload_size = initial_size - sizeof(block_header);
    free_list->is_free = 1;
    free_list->next = NULL;
    free_list->prev = NULL;
    free_list->magic = MAGIC_FREE;
    
    return heap_start;
}

// Find a suitable free block using first-fit strategy
block_header* find_free_block(size_t required_size) {
    block_header* current = free_list;
    
    while (current != NULL) {
        if (current->is_free && current->payload_size >= required_size) {
            return current;
        }
        current = current->next;
    }
    
    return NULL;
}

// Split a block if there's enough leftover space
block_header* split_block(block_header* block, size_t required_size) {
    if (!block) return NULL;
    
    size_t total_available = block->payload_size;
    size_t leftover_size = total_available - required_size;
    
    // Only split if leftover is large enough for a meaningful block
    if (leftover_size < sizeof(block_header) + MIN_PAYLOAD_SIZE) {
        return NULL; // Don't split, use the entire block
    }
    
    // Create the new block at the split point
    char* split_point = (char*)block + sizeof(block_header) + required_size;
    block_header* new_block = (block_header*)split_point;
    
    // Set up the new block
    new_block->payload_size = leftover_size - sizeof(block_header);
    new_block->is_free = 1;
    new_block->magic = MAGIC_FREE;
    
    // Insert new block into the linked list
    new_block->next = block->next;
    new_block->prev = block;
    
    if (block->next) {
        block->next->prev = new_block;
    }
    block->next = new_block;
    
    // Update the original block size
    block->payload_size = required_size;
    
    return new_block;
}

// Add a block to the free list
void add_to_free_list(block_header* block) {
    if (!block) return;
    
    block->is_free = 1;
    block->magic = MAGIC_FREE;
    
    // Insert at the beginning of free list for simplicity
    block->next = free_list;
    block->prev = NULL;
    
    if (free_list) {
        free_list->prev = block;
    }
    
    free_list = block;
}

// Remove a block from the free list
void remove_from_free_list(block_header* block) {
    if (!block) return;
    
    if (block->prev) {
        block->prev->next = block->next;
    } else {
        // This block was the head of free_list
        free_list = block->next;
    }
    
    if (block->next) {
        block->next->prev = block->prev;
    }
    
    block->next = NULL;
    block->prev = NULL;
}

// Expand the heap when no suitable free blocks are found
void* expand_heap(size_t size) {
    size_t expand_size = (size > DEFAULT_HEAP_SIZE) ? align_size(size) : DEFAULT_HEAP_SIZE;
    
    void* old_end = heap_end;
    if (sbrk(expand_size) == (void*)-1) {
        return NULL;
    }
    
    heap_end = sbrk(0);
    
    // Create a new block at the old heap end
    block_header* new_block = (block_header*)old_end;
    new_block->payload_size = expand_size - sizeof(block_header);
    new_block->is_free = 1;
    new_block->magic = MAGIC_FREE;
    new_block->next = NULL;
    new_block->prev = NULL;
    
    // Try to coalesce with the last block if it's free
    // Find the last allocated block
    block_header* current = (block_header*)heap_start;
    block_header* last_block = NULL;
    
    while ((char*)current < (char*)old_end) {
        last_block = current;
        current = (block_header*)((char*)current + sizeof(block_header) + current->payload_size);
    }
    
    if (last_block && last_block->is_free) {
        // Coalesce with the last block
        last_block->payload_size += expand_size;
        return last_block;
    } else {
        // Add the new block to free list
        add_to_free_list(new_block);
        return new_block;
    }
}

// Main malloc implementation
void* my_malloc(size_t size) {
    if (size == 0) {
        return NULL;
    }
    
    // Initialize heap if not done already
    if (heap_start == NULL) {
        if (init_allocator(DEFAULT_HEAP_SIZE) == NULL) {
            return NULL;
        }
    }
    
    // Align the requested size
    size = align_size(size);
    
    // Find a suitable free block
    block_header* block = find_free_block(size);
    
    // If no suitable block found, expand the heap
    if (!block) {
        block = expand_heap(size + sizeof(block_header));
        if (!block) {
            return NULL;
        }
    }
    
    // Remove the block from free list
    remove_from_free_list(block);
    
    // Split the block if there's enough leftover space
    block_header* leftover = split_block(block, size);
    
    // If we split the block, add the leftover to free list
    if (leftover) {
        add_to_free_list(leftover);
    }
    
    // Mark the block as allocated
    block->is_free = 0;
    block->magic = MAGIC_ALLOCATED;
    
    // Return pointer to the payload
    return (char*)block + sizeof(block_header);
}

// Coalesce adjacent free blocks
void coalesce_block(block_header* block) {
    if (!block || !block->is_free) return;
    
    // Find all blocks in memory order to coalesce properly
    block_header* current = (block_header*)heap_start;
    
    while ((char*)current < (char*)heap_end) {
        if (current == block) {
            // Try to coalesce with next block
            block_header* next_in_memory = (block_header*)((char*)current + sizeof(block_header) + current->payload_size);
            
            if ((char*)next_in_memory < (char*)heap_end && next_in_memory->is_free) {
                // Remove next block from free list
                remove_from_free_list(next_in_memory);
                
                // Coalesce
                current->payload_size += sizeof(block_header) + next_in_memory->payload_size;
            }
            break;
        }
        current = (block_header*)((char*)current + sizeof(block_header) + current->payload_size);
    }
    
    // Now try to coalesce with previous block
    current = (block_header*)heap_start;
    block_header* prev_block = NULL;
    
    while ((char*)current < (char*)heap_end && current != block) {
        prev_block = current;
        current = (block_header*)((char*)current + sizeof(block_header) + current->payload_size);
    }
    
    if (prev_block && prev_block->is_free) {
        block_header* next_expected = (block_header*)((char*)prev_block + sizeof(block_header) + prev_block->payload_size);
        
        if (next_expected == block) {
            // Remove current block from free list
            remove_from_free_list(block);
            
            // Coalesce with previous
            prev_block->payload_size += sizeof(block_header) + block->payload_size;
        }
    }
}

// Main free implementation
void my_free(void* payload_ptr) {
    if (!payload_ptr) return;
    
    // Get the block header
    block_header* block = (block_header*)((char*)payload_ptr - sizeof(block_header));
    
    // Validate the block
    if (block->magic != MAGIC_ALLOCATED) {
        fprintf(stderr, "Error: Invalid free - corrupted block or double free\n");
        return;
    }
    
    // Mark as free
    block->is_free = 1;
    block->magic = MAGIC_FREE;
    
    // Add to free list
    add_to_free_list(block);
    
    // Coalesce with adjacent free blocks
    coalesce_block(block);
}

// Validate heap integrity (for debugging)
int validate_heap() {
    if (!heap_start) return 1; // Empty heap is valid
    
    block_header* current = (block_header*)heap_start;
    
    while ((char*)current < (char*)heap_end) {
        // Check magic number
        if (current->magic != MAGIC_FREE && current->magic != MAGIC_ALLOCATED) {
            fprintf(stderr, "Heap corruption detected: invalid magic number\n");
            return 0;
        }
        
        // Check if block extends beyond heap
        if ((char*)current + sizeof(block_header) + current->payload_size > (char*)heap_end) {
            fprintf(stderr, "Heap corruption detected: block extends beyond heap\n");
            return 0;
        }
        
        current = (block_header*)((char*)current + sizeof(block_header) + current->payload_size);
    }
    
    return 1; // Heap is valid
}

// Debug function to print heap state
void print_heap_debug() {
    printf("=== Heap Debug Info ===\n");
    printf("Heap start: %p\n", heap_start);
    printf("Heap end: %p\n", heap_end);
    printf("Heap size: %zu bytes\n", (char*)heap_end - (char*)heap_start);
    
    if (!heap_start) {
        printf("Heap not initialized\n");
        return;
    }
    
    printf("\nBlocks in memory:\n");
    block_header* current = (block_header*)heap_start;
    int block_num = 0;
    
    while ((char*)current < (char*)heap_end) {
        printf("Block %d: addr=%p, size=%zu, free=%s, magic=0x%x\n",
               block_num++, current, current->payload_size,
               current->is_free ? "yes" : "no", current->magic);
        
        current = (block_header*)((char*)current + sizeof(block_header) + current->payload_size);
    }
    
    printf("\nFree list:\n");
    current = free_list;
    block_num = 0;
    while (current) {
        printf("Free block %d: addr=%p, size=%zu\n",
               block_num++, current, current->payload_size);
        current = current->next;
    }
    printf("======================\n\n");
}