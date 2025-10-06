// kernel/pmm.c
// Physical Memory Manager Implementation
// Manages physical RAM using a bitmap allocator
// Each bit represents one 4KB page (0 = free, 1 = used)
//
// Created by: floof<3

#include "pmm.h"
#include "../drivers/serial.h"

// Bitmap to track page usage (one bit per page)
static uint8_t* page_bitmap = NULL;
static uint64_t total_pages = 0;
static uint64_t used_pages = 0;

// Kernel end address (defined in linker script)
extern uint64_t _kernel_end;

// Set a bit in the bitmap (mark page as used)
static void bitmap_set(uint64_t page) {
    uint64_t byte = page / 8;
    uint64_t bit = page % 8;
    page_bitmap[byte] |= (1 << bit);
}

// Clear a bit in the bitmap (mark page as free)
static void bitmap_clear(uint64_t page) {
    uint64_t byte = page / 8;
    uint64_t bit = page % 8;
    page_bitmap[byte] &= ~(1 << bit);
}

// Test if a bit is set (check if page is used)
static bool bitmap_test(uint64_t page) {
    uint64_t byte = page / 8;
    uint64_t bit = page % 8;
    return (page_bitmap[byte] & (1 << bit)) != 0;
}

// Initialize the physical memory manager
void pmm_init(uint64_t total_memory) {
    serial_write("PMM: Initializing physical memory manager...\n");
    
    // Calculate how many pages we have
    total_pages = total_memory / PAGE_SIZE;
    
    // Calculate bitmap size (one bit per page, so divide by 8 to get bytes)
    uint64_t bitmap_size = total_pages / 8;
    if (total_pages % 8) bitmap_size++;  // Round up if not exact multiple
    
    // Place bitmap right after the kernel in memory
    // This is a bit sketchy but works for now (we'll improve this later)
    page_bitmap = (uint8_t*)&_kernel_end;
    
    // Clear the entire bitmap (mark all pages as free initially)
    for (uint64_t i = 0; i < bitmap_size; i++) {
        page_bitmap[i] = 0;
    }
    
    // Mark pages used by the kernel and bitmap as occupied
    uint64_t kernel_pages = ((uint64_t)&_kernel_end) / PAGE_SIZE;
    uint64_t bitmap_pages = bitmap_size / PAGE_SIZE;
    if (bitmap_size % PAGE_SIZE) bitmap_pages++;
    
    for (uint64_t i = 0; i < kernel_pages + bitmap_pages; i++) {
        bitmap_set(i);
        used_pages++;
    }
    
    // Mark first 1MB as used (BIOS data, VGA memory, etc.)
    // Never allocate from here or shit will break
    for (uint64_t i = 0; i < (0x100000 / PAGE_SIZE); i++) {
        bitmap_set(i);
    }
    
    serial_write("PMM: Initialization complete\n");
    serial_write("PMM: Total pages: ");
    // TODO: Add number printing function
    serial_write("\n");
}

// Allocate a physical page
void* pmm_alloc_page(void) {
    // Search through bitmap for a free page
    for (uint64_t i = 0; i < total_pages; i++) {
        if (!bitmap_test(i)) {
            // Found a free page!
            bitmap_set(i);
            used_pages++;
            
            // Return physical address of the page
            return (void*)(i * PAGE_SIZE);
        }
    }
    
    // Out of memory (this is bad news)
    serial_write("PMM: ERROR - Out of memory!\n");
    return NULL;
}

// Free a physical page
void pmm_free_page(void* page) {
    // Calculate page number from physical address
    uint64_t page_num = (uint64_t)page / PAGE_SIZE;
    
    // Make sure it's actually allocated before freeing
    if (!bitmap_test(page_num)) {
        serial_write("PMM: WARNING - Attempted to free already-free page\n");
        return;
    }
    
    // Mark as free
    bitmap_clear(page_num);
    used_pages--;
}

// Get total number of pages
uint64_t pmm_get_total_pages(void) {
    return total_pages;
}

// Get number of free pages
uint64_t pmm_get_free_pages(void) {
    return total_pages - used_pages;
}

// Get number of used pages
uint64_t pmm_get_used_pages(void) {
    return used_pages;
}
