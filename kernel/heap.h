// kernel/heap.h
// Heap memory allocator - kmalloc/kfree for dynamic memory allocation
// This is like malloc/free but for the kernel
//
// Created by: floof<3

#ifndef HEAP_H
#define HEAP_H

#include <stdint.h>
#include <stddef.h>

// Initialize the kernel heap
void heap_init(void);

// Allocate memory from the heap (like malloc)
// Returns NULL if out of memory
void* kmalloc(size_t size);

// Free memory back to the heap (like free)
void kfree(void* ptr);

// Allocate aligned memory (useful for page-aligned allocations)
void* kmalloc_aligned(size_t size, size_t alignment);

#endif // HEAP_H
