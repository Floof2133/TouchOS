#ifndef MEMORY_H
#define MEMORY_H

#include <stdint.h>
#include <stddef.h>

typedef struct {
    uint32_t Type;
    uint64_t PhysicalStart;
    uint64_t VirtualStart;
    uint64_t NumberOfPages;
    uint64_t Attribute;
} EFI_MEMORY_DESCRIPTOR;

typedef struct {
    uint32_t lock;
} spinlock_t;

void spin_lock(spinlock_t* lock);
void spin_unlock(spinlock_t* lock);
void* memset(void* dest, int val, size_t count);
void* memcpy(void* dest, const void* src, size_t count);
void* kmalloc(size_t size);
void kfree(void* ptr);

#endif
