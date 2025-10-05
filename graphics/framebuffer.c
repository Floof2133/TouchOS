// graphics/framebuffer.c
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
    fb.address = (uint32_t*)gop->Mode->FrameBufferBase;
    fb.width = gop->Mode->Info->HorizontalResolution;
    fb.height = gop->Mode->Info->VerticalResolution;
    fb.pitch = gop->Mode->Info->PixelsPerScanLine * 4;
    fb.bpp = 32;
    
    // Allocate backbuffer for double buffering
    size_t buffer_size = fb.height * fb.pitch;
    fb.backbuffer = (uint32_t*)kmalloc(buffer_size);
    memcpy(fb.backbuffer, fb.address, buffer_size);
    
    // Initialize hardware cursor if supported
    if (gop->Mode->Info->PixelFormat == PixelBlueGreenRedReserved8BitPerColor) {
        fb.cursor_data = kmalloc(64 * 64 * 4);  // 64x64 cursor
        framebuffer_create_default_cursor();
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

// Accelerated graphics operations using SIMD
void framebuffer_blit_alpha_sse2(uint32_t* dst, uint32_t* src, 
                                 int width, int height, 
                                 int dst_stride, int src_stride) {
    __m128i alpha_mask = _mm_set1_epi32(0xFF000000);
    __m128i one = _mm_set1_epi16(0x0101);
    
    for (int y = 0; y < height; y++) {
        uint32_t* dst_row = dst + y * dst_stride;
        uint32_t* src_row = src + y * src_stride;
        
        for (int x = 0; x < width; x += 4) {
            __m128i src_pixel = _mm_loadu_si128((__m128i*)(src_row + x));
            __m128i dst_pixel = _mm_loadu_si128((__m128i*)(dst_row + x));
            
            // Extract alpha channel
            __m128i alpha = _mm_and_si128(src_pixel, alpha_mask);
            alpha = _mm_srli_epi32(alpha, 24);
            
            // Alpha blending: dst = src * alpha + dst * (1 - alpha)
            __m128i inv_alpha = _mm_sub_epi32(_mm_set1_epi32(255), alpha);
            
            // Unpack to 16-bit for multiplication
            __m128i src_lo = _mm_unpacklo_epi8(src_pixel, _mm_setzero_si128());
            __m128i src_hi = _mm_unpackhi_epi8(src_pixel, _mm_setzero_si128());
            __m128i dst_lo = _mm_unpacklo_epi8(dst_pixel, _mm_setzero_si128());
            __m128i dst_hi = _mm_unpackhi_epi8(dst_pixel, _mm_setzero_si128());
            
            // Multiply and blend
            src_lo = _mm_mullo_epi16(src_lo, _mm_set1_epi16(alpha[0]));
            src_hi = _mm_mullo_epi16(src_hi, _mm_set1_epi16(alpha[2]));
            dst_lo = _mm_mullo_epi16(dst_lo, _mm_set1_epi16(inv_alpha[0]));
            dst_hi = _mm_mullo_epi16(dst_hi, _mm_set1_epi16(inv_alpha[2]));
            
            // Add and pack back to 8-bit
            __m128i result_lo = _mm_add_epi16(src_lo, dst_lo);
            __m128i result_hi = _mm_add_epi16(src_hi, dst_hi);
            result_lo = _mm_srli_epi16(result_lo, 8);
            result_hi = _mm_srli_epi16(result_hi, 8);
            
            __m128i result = _mm_packus_epi16(result_lo, result_hi);
            _mm_storeu_si128((__m128i*)(dst_row + x), result);
        }
    }
}
