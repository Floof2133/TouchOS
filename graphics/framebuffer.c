// graphics/framebuffer.c
#include <stdint.h>
#include <stddef.h>
#include <stdbool.h>
#include "../kernel/heap.h"

// Missing type definitions
typedef struct {
    int x, y, width, height;
} rect_t;

typedef struct window {
    rect_t bounds;
    struct window* next;
    // Other fields can be added as needed
} window_t;

typedef struct {
    volatile int locked;
} spinlock_t;

typedef void* EFI_GRAPHICS_OUTPUT_PROTOCOL;

static inline void spin_lock(spinlock_t* lock) {
    while (__sync_lock_test_and_set(&lock->locked, 1)) {
        __asm__ volatile("pause");
    }
}

static inline void spin_unlock(spinlock_t* lock) {
    __sync_lock_release(&lock->locked);
}

static inline void* memcpy(void* dest, const void* src, size_t n) {
    uint8_t* d = dest;
    const uint8_t* s = src;
    for (size_t i = 0; i < n; i++) d[i] = s[i];
    return dest;
}

static inline bool rect_intersects(rect_t* a, rect_t* b) {
    return !(a->x + a->width < b->x || b->x + b->width < a->x ||
             a->y + a->height < b->y || b->y + b->height < a->y);
}

// Forward declarations
void window_composite(window_t* win, uint32_t* buffer, rect_t* rect);
void framebuffer_wait_vsync(void);
void framebuffer_create_default_cursor(void);

// Placeholder window list
static window_t* window_list = NULL;

// Placeholder for missing pixel format enum
enum {
    PixelBlueGreenRedReserved8BitPerColor = 1
};

typedef struct {
    uint32_t* address;
    uint32_t width;
    uint32_t height;
    uint32_t pitch;
    uint32_t bpp;
    
    // Double buffering
    uint32_t* backbuffer;
    spinlock_t flip_lock;
    
    // Hardware cursor
    uint32_t* cursor_data;
    int cursor_x;
    int cursor_y;
} framebuffer_t;

static framebuffer_t fb = {0};

void framebuffer_init(EFI_GRAPHICS_OUTPUT_PROTOCOL* gop) {
    // TODO: Properly parse GOP structure when available
    (void)gop;

    // For now, use placeholder values
    fb.address = NULL;  // Will be set by bootloader
    fb.width = 1920;
    fb.height = 1080;
    fb.pitch = 1920 * 4;
    fb.bpp = 32;

    // Allocate backbuffer for double buffering
    if (fb.address) {
        size_t buffer_size = fb.height * fb.pitch;
        fb.backbuffer = (uint32_t*)kmalloc(buffer_size);
        if (fb.backbuffer) {
            memcpy(fb.backbuffer, fb.address, buffer_size);
        }

        // Allocate cursor data
        fb.cursor_data = kmalloc(64 * 64 * 4);  // 64x64 cursor
        if (fb.cursor_data) {
            framebuffer_create_default_cursor();
        }
    }
}

// Compositor with damage tracking
typedef struct {
    rect_t* damage_rects;
    int damage_count;
    int damage_capacity;
    spinlock_t lock;
} compositor_t;

static compositor_t compositor = {0};

void compositor_init(void) {
    compositor.damage_capacity = 100;
    compositor.damage_rects = kmalloc(sizeof(rect_t) * compositor.damage_capacity);
    compositor.damage_count = 0;
}

void compositor_damage_region(int x, int y, int width, int height) {
    spin_lock(&compositor.lock);
    
    if (compositor.damage_count < compositor.damage_capacity) {
        compositor.damage_rects[compositor.damage_count++] = (rect_t){
            .x = x, .y = y, .width = width, .height = height
        };
    } else {
        // Merge all damage into full screen update
        compositor.damage_count = 1;
        compositor.damage_rects[0] = (rect_t){
            .x = 0, .y = 0, .width = fb.width, .height = fb.height
        };
    }
    
    spin_unlock(&compositor.lock);
}

void compositor_composite(void) {
    spin_lock(&compositor.lock);
    
    // Composite only damaged regions
    for (int i = 0; i < compositor.damage_count; i++) {
        rect_t* rect = &compositor.damage_rects[i];
        
        // Clip to screen bounds
        if (rect->x < 0) { rect->width += rect->x; rect->x = 0; }
        if (rect->y < 0) { rect->height += rect->y; rect->y = 0; }
        if (rect->x + rect->width > fb.width) rect->width = fb.width - rect->x;
        if (rect->y + rect->height > fb.height) rect->height = fb.height - rect->y;
        
        // Composite windows in this region
        for (window_t* win = window_list; win; win = win->next) {
            if (rect_intersects(&win->bounds, rect)) {
                window_composite(win, fb.backbuffer, rect);
            }
        }
    }
    
    // Swap buffers (vsync if available)
    framebuffer_wait_vsync();
    
    spin_lock(&fb.flip_lock);
    
    // Copy only damaged regions to front buffer
    for (int i = 0; i < compositor.damage_count; i++) {
        rect_t* rect = &compositor.damage_rects[i];
        
        for (int y = rect->y; y < rect->y + rect->height; y++) {
            uint32_t* src = fb.backbuffer + y * (fb.pitch / 4) + rect->x;
            uint32_t* dst = fb.address + y * (fb.pitch / 4) + rect->x;
            memcpy(dst, src, rect->width * 4);
        }
    }
    
    spin_unlock(&fb.flip_lock);
    
    // Clear damage list
    compositor.damage_count = 0;
    
    spin_unlock(&compositor.lock);
}

// Stub implementations for missing functions
void framebuffer_wait_vsync(void) {
    // TODO: Implement vsync waiting
}

void framebuffer_create_default_cursor(void) {
    // TODO: Create default cursor
}

void window_composite(window_t* win, uint32_t* buffer, rect_t* rect) {
    (void)win; (void)buffer; (void)rect;
    // TODO: Implement window compositing
}

// Accelerated graphics operations using SIMD (disabled for now - needs proper includes)
void framebuffer_blit_alpha_sse2(uint32_t* dst, uint32_t* src,
                                 int width, int height,
                                 int dst_stride, int src_stride) {
    // TODO: Implement SSE2 alpha blending (needs emmintrin.h)
    (void)dst; (void)src; (void)width; (void)height;
    (void)dst_stride; (void)src_stride;
}
