// kernel/pmm.h
// Physical Memory Manager - keeps track of which RAM pages are free/used
// Think of it like a parking lot attendant but for memory pages
//
// Created by: floof<3

#ifndef PMM_H
#define PMM_H

#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>

// Page size (4KB is standard for x86_64)
#define PAGE_SIZE 4096

// Bitmap-based allocator (each bit = one 4KB page)
// If bit is 0 = page is free, if bit is 1 = page is used
// Simple but effective (and doesn't waste much memory)

// Initialize physical memory manager
// total_memory = how many bytes of RAM we have (get this from GRUB multiboot info)
void pmm_init(uint64_t total_memory);

// Allocate a physical page (returns physical address)
// Returns 0 if out of memory (which would be really bad)
void* pmm_alloc_page(void);

// Free a physical page (mark it as available again)
void pmm_free_page(void* page);

// Get total number of pages
uint64_t pmm_get_total_pages(void);

// Get number of free pages (useful for debugging memory leaks)
uint64_t pmm_get_free_pages(void);

// Get number of used pages
uint64_t pmm_get_used_pages(void);

#endif // PMM_H
