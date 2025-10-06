// kernel/heap.c
// Kernel heap allocator implementation
// Uses a simple free-list allocator (not the fastest but it gets the job done)
// Think of it like a parking lot manager but for memory chunks
//
// Created by: floof<3

#include "heap.h"
#include "pmm.h"
#include "../drivers/serial.h"

// Block header for tracking allocated memory
// Every allocation gets a header with size and status info
// It's like a label on a box that says "this box is 256 bytes and currently being used"
typedef struct heap_block {
    size_t size;                // Size of this block (not including header, obviously)
    bool is_free;               // Is this block free or allocated? (true = free parking spot)
    struct heap_block* next;    // Next block in the list (linked list gang gang)
} heap_block_t;

// Start of the heap (first block in the chain)
static heap_block_t* heap_start = NULL;

// Initialize the kernel heap
void heap_init(void) {
    serial_write("Heap: Initializing kernel heap...\n");
    
    // Allocate first page for heap (gotta start somewhere)
    void* first_page = pmm_alloc_page();
    if (!first_page) {
        serial_write("Heap: ERROR - Failed to allocate initial page!\n");
        return;  // Well fuck, that's not good
    }
    
    // Set up first block (initially one big free block)
    // It's like having an empty parking lot - one giant space
    heap_start = (heap_block_t*)first_page;
    heap_start->size = PAGE_SIZE - sizeof(heap_block_t);
    heap_start->is_free = true;
    heap_start->next = NULL;  // No other blocks yet
    
    serial_write("Heap: Initialization complete\n");
}

// Find a free block that's big enough (first-fit algorithm)
// We just walk the list and take the first one that fits (not fancy but it works)
static heap_block_t* find_free_block(size_t size) {
    heap_block_t* current = heap_start;
    
    while (current) {
        if (current->is_free && current->size >= size) {
            return current;  // Found one! Lucky us
        }
        current = current->next;
    }
    
    return NULL;  // No suitable block found (sad times)
}

// Split a block if it's too big (avoid wasting memory like a dumbass)
static void split_block(heap_block_t* block, size_t size) {
    // Only split if there's enough space left over for a new block
    // No point in creating a 16-byte free block, that's just wasteful
    if (block->size > size + sizeof(heap_block_t) + 16) {
        // Create a new block in the leftover space
        heap_block_t* new_block = (heap_block_t*)((uint8_t*)block + sizeof(heap_block_t) + size);
        new_block->size = block->size - size - sizeof(heap_block_t);
        new_block->is_free = true;
        new_block->next = block->next;
        
        // Shrink the original block to fit
        block->size = size;
        block->next = new_block;
    }
}

// Allocate memory from the heap (this is the money maker)
void* kmalloc(size_t size) {
    if (size == 0) return NULL;  // Allocating 0 bytes? Really?
    
    // Align size to 8 bytes (makes things faster and cleaner)
    // CPU likes aligned data, we like CPU being happy
    size = (size + 7) & ~7;
    
    // Try to find a free block that fits
    heap_block_t* block = find_free_block(size);
    
    if (!block) {
        // No free block big enough, need to expand heap
        // Time to ask PMM for more pages (like asking for more parking spaces)
        size_t pages_needed = (size + sizeof(heap_block_t) + PAGE_SIZE - 1) / PAGE_SIZE;
        
        // Find last block in the chain
        heap_block_t* last = heap_start;
        while (last->next) last = last->next;
        
        // Allocate the pages we need
        for (size_t i = 0; i < pages_needed; i++) {
            void* page = pmm_alloc_page();
            if (!page) {
                serial_write("Heap: ERROR - Out of memory!\n");
                return NULL;  // We're fucked, out of RAM
            }
            
            // Add new block at the end of the list
            heap_block_t* new_block = (heap_block_t*)page;
            new_block->size = PAGE_SIZE - sizeof(heap_block_t);
            new_block->is_free = true;
            new_block->next = NULL;
            
            last->next = new_block;
            last = new_block;
        }
        
        // Try again to find a block (should work now)
        block = find_free_block(size);
        if (!block) return NULL;  // Still failed somehow? wtf
    }
    
    // Split the block if it's way too big (don't waste memory like an idiot)
    split_block(block, size);
    
    // Mark as allocated (this parking spot is now taken)
    block->is_free = false;
    
    // Return pointer to data (skip the header, user doesn't need to see that shit)
    return (void*)((uint8_t*)block + sizeof(heap_block_t));
}

// Free memory back to the heap (give the parking spot back)
void kfree(void* ptr) {
    if (!ptr) return;  // Freeing NULL is a no-op (like regular free, we're not assholes about it)
    
    // Get block header (it's right before the data pointer)
    // We hid it there earlier like a sneaky bastard
    heap_block_t* block = (heap_block_t*)((uint8_t*)ptr - sizeof(heap_block_t));
    
    // Mark as free (parking spot available!)
    block->is_free = true;
    
    // Coalesce with next block if it's also free (merge adjacent free blocks)
    // It's like combining two empty parking spots into one big one
    if (block->next && block->next->is_free) {
        block->size += sizeof(heap_block_t) + block->next->size;
        block->next = block->next->next;
    }
    
    // TODO: Could also coalesce with previous block but that requires a doubly-linked list
    // Maybe later when we're not lazy
}

// Allocate aligned memory (for when you need shit aligned to specific boundaries)
void* kmalloc_aligned(size_t size, size_t alignment) {
    // Allocate extra space for alignment adjustment (we'll waste a bit but whatever)
    void* ptr = kmalloc(size + alignment + sizeof(void*));
    if (!ptr) return NULL;
    
    // Calculate aligned address (bit twiddling magic)
    uintptr_t addr = (uintptr_t)ptr;
    uintptr_t aligned = (addr + alignment - 1) & ~(alignment - 1);
    
    // Store original pointer before aligned address (so we can free it later)
    // This is important or we'll leak memory like a sieve
    *((void**)(aligned - sizeof(void*))) = ptr;
    
    return (void*)aligned;
}
