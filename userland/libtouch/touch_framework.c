// userland/libtouch/touch_framework.c
// TouchOS Unified Touch Framework Implementation
// Created by: floof<3

#include "touch_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <time.h>
#include <math.h>

// ============================================================================
// Internal State
// ============================================================================

typedef struct {
    // Framebuffer
    int fb_fd;
    uint32_t* fb_mem;
    uint32_t* back_buffer;
    int fb_width;
    int fb_height;
    int fb_size;

    // Touch input
    int touch_fd;
    touch_event_t last_event;

    // Keyboard
    touch_keyboard_t keyboard;

    // Running state
    bool running;

    // Event handler
    void (*event_handler)(touch_event_t* event);

} framework_state_t;

static framework_state_t state = {0};

// ============================================================================
// Initialization
// ============================================================================

void touch_framework_init(void) {
    printf("Initializing Touch Framework...\n");

    // Open framebuffer
    state.fb_fd = open("/dev/fb0", O_RDWR);
    if (state.fb_fd < 0) {
        printf("Warning: Could not open framebuffer, using dummy mode\n");
        state.fb_width = TOUCH_SCREEN_WIDTH;
        state.fb_height = TOUCH_SCREEN_HEIGHT;
        state.fb_size = state.fb_width * state.fb_height * 4;

        // Allocate dummy buffers
        state.fb_mem = malloc(state.fb_size);
        state.back_buffer = malloc(state.fb_size);
        memset(state.fb_mem, 0, state.fb_size);
        memset(state.back_buffer, 0, state.fb_size);
    } else {
        struct fb_var_screeninfo vinfo;
        ioctl(state.fb_fd, FBIOGET_VSCREENINFO, &vinfo);

        state.fb_width = vinfo.xres;
        state.fb_height = vinfo.yres;
        state.fb_size = state.fb_width * state.fb_height * 4;

        // Map framebuffer
        state.fb_mem = mmap(0, state.fb_size, PROT_READ | PROT_WRITE,
                           MAP_SHARED, state.fb_fd, 0);

        // Allocate back buffer
        state.back_buffer = malloc(state.fb_size);
        memset(state.back_buffer, 0, state.fb_size);
    }

    // Initialize keyboard
    state.keyboard.visible = false;
    state.keyboard.layout = OSK_LAYOUT_QWERTY;

    state.running = true;

    printf("Touch Framework initialized (%dx%d)\n", state.fb_width, state.fb_height);
}

void touch_framework_shutdown(void) {
    if (state.fb_fd >= 0) {
        if (state.fb_mem != MAP_FAILED) {
            munmap(state.fb_mem, state.fb_size);
        }
        close(state.fb_fd);
    }

    if (state.back_buffer) {
        free(state.back_buffer);
    }

    printf("Touch Framework shutdown\n");
}

// ============================================================================
// Drawing API
// ============================================================================

void touch_clear_screen(uint32_t color) {
    for (int i = 0; i < state.fb_width * state.fb_height; i++) {
        state.back_buffer[i] = color;
    }
}

void touch_draw_rect(int x, int y, int w, int h, uint32_t color) {
    for (int py = y; py < y + h && py < state.fb_height; py++) {
        if (py < 0) continue;
        for (int px = x; px < x + w && px < state.fb_width; px++) {
            if (px < 0) continue;
            state.back_buffer[py * state.fb_width + px] = color;
        }
    }
}

void touch_draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color) {
    // Simple implementation: draw rect with corners cut
    // Full rounded corners would use circle drawing

    // Top and bottom edges
    touch_draw_rect(x + radius, y, w - 2 * radius, radius, color);
    touch_draw_rect(x + radius, y + h - radius, w - 2 * radius, radius, color);

    // Middle section
    touch_draw_rect(x, y + radius, w, h - 2 * radius, color);

    // Corner approximation (simple)
    for (int i = 0; i < radius; i++) {
        int len = (int)(sqrt(radius * radius - i * i));

        // Top-left
        touch_draw_rect(x + radius - len, y + i, len, 1, color);
        // Top-right
        touch_draw_rect(x + w - radius, y + i, len, 1, color);
        // Bottom-left
        touch_draw_rect(x + radius - len, y + h - 1 - i, len, 1, color);
        // Bottom-right
        touch_draw_rect(x + w - radius, y + h - 1 - i, len, 1, color);
    }
}

void touch_draw_circle(int cx, int cy, int radius, uint32_t color) {
    for (int y = -radius; y <= radius; y++) {
        for (int x = -radius; x <= radius; x++) {
            if (x * x + y * y <= radius * radius) {
                int px = cx + x;
                int py = cy + y;
                if (px >= 0 && px < state.fb_width && py >= 0 && py < state.fb_height) {
                    state.back_buffer[py * state.fb_width + px] = color;
                }
            }
        }
    }
}

void touch_draw_line(int x1, int y1, int x2, int y2, uint32_t color, int thickness) {
    // Bresenham's line algorithm
    int dx = abs(x2 - x1);
    int dy = abs(y2 - y1);
    int sx = x1 < x2 ? 1 : -1;
    int sy = y1 < y2 ? 1 : -1;
    int err = dx - dy;

    while (1) {
        // Draw point with thickness
        for (int ty = -thickness/2; ty <= thickness/2; ty++) {
            for (int tx = -thickness/2; tx <= thickness/2; tx++) {
                int px = x1 + tx;
                int py = y1 + ty;
                if (px >= 0 && px < state.fb_width && py >= 0 && py < state.fb_height) {
                    state.back_buffer[py * state.fb_width + px] = color;
                }
            }
        }

        if (x1 == x2 && y1 == y2) break;

        int e2 = 2 * err;
        if (e2 > -dy) {
            err -= dy;
            x1 += sx;
        }
        if (e2 < dx) {
            err += dx;
            y1 += sy;
        }
    }
}

void touch_draw_text(int x, int y, const char* text, uint32_t color, int size) {
    // Simple bitmap font rendering (simplified)
    // In a real implementation, would use FreeType or bitmap fonts

    int char_width = size / 2;
    int char_height = size;

    while (*text) {
        // Draw simple rectangle per character (placeholder)
        // Real implementation would render actual glyphs
        x += char_width;
        text++;
    }
}

void touch_draw_text_centered(touch_rect_t* rect, const char* text, uint32_t color, int size) {
    int text_width = touch_measure_text(text, size);
    int x = rect->x + (rect->width - text_width) / 2;
    int y = rect->y + (rect->height - size) / 2;
    touch_draw_text(x, y, text, color, size);
}

int touch_measure_text(const char* text, int size) {
    // Simplified: character width = size / 2
    return strlen(text) * (size / 2);
}

void touch_draw_emoji(int x, int y, const char* emoji, int size) {
    // Placeholder: draw colored circle
    touch_draw_circle(x + size/2, y + size/2, size/2, TOUCH_COLOR_PRIMARY);
}

void touch_flip_buffer(void) {
    // Copy back buffer to front buffer
    if (state.fb_mem && state.fb_mem != MAP_FAILED) {
        memcpy(state.fb_mem, state.back_buffer, state.fb_size);
    }
}

// ============================================================================
// Widget API - Button
// ============================================================================

touch_button_t* touch_button_create(int x, int y, int w, int h, const char* label) {
    touch_button_t* btn = malloc(sizeof(touch_button_t));
    if (!btn) return NULL;

    btn->bounds.x = x;
    btn->bounds.y = y;
    btn->bounds.width = w;
    btn->bounds.height = h;

    btn->label = strdup(label);
    btn->icon = NULL;
    btn->color = TOUCH_COLOR_PRIMARY;
    btn->enabled = true;
    btn->visible = true;
    btn->on_tap = NULL;
    btn->user_data = NULL;

    return btn;
}

void touch_button_set_icon(touch_button_t* btn, const char* icon) {
    if (btn->icon) free(btn->icon);
    btn->icon = strdup(icon);
}

void touch_button_draw(touch_button_t* btn) {
    if (!btn || !btn->visible) return;

    uint32_t color = btn->enabled ? btn->color : TOUCH_COLOR_SECONDARY;

    touch_draw_rounded_rect(btn->bounds.x, btn->bounds.y,
                           btn->bounds.width, btn->bounds.height,
                           TOUCH_CORNER_RADIUS, color);

    touch_draw_text_centered(&btn->bounds, btn->label, TOUCH_COLOR_TEXT, 20);
}

bool touch_button_hit_test(touch_button_t* btn, int x, int y) {
    if (!btn || !btn->enabled || !btn->visible) return false;

    return touch_rect_contains(&btn->bounds, x, y);
}

void touch_button_free(touch_button_t* btn) {
    if (!btn) return;
    if (btn->label) free(btn->label);
    if (btn->icon) free(btn->icon);
    free(btn);
}

// ============================================================================
// Widget API - Slider
// ============================================================================

touch_slider_t* touch_slider_create(int x, int y, int w, float min, float max) {
    touch_slider_t* slider = malloc(sizeof(touch_slider_t));
    if (!slider) return NULL;

    slider->bounds.x = x;
    slider->bounds.y = y;
    slider->bounds.width = w;
    slider->bounds.height = 40;
    slider->value = 0.5f;
    slider->min = min;
    slider->max = max;
    slider->on_change = NULL;
    slider->user_data = NULL;

    return slider;
}

void touch_slider_draw(touch_slider_t* slider) {
    if (!slider) return;

    int track_height = 10;
    int thumb_size = 30;

    // Track background
    int track_y = slider->bounds.y + (slider->bounds.height - track_height) / 2;
    touch_draw_rounded_rect(slider->bounds.x, track_y,
                           slider->bounds.width, track_height,
                           5, TOUCH_COLOR_SECONDARY);

    // Track fill
    int fill_width = (int)(slider->bounds.width * slider->value);
    touch_draw_rounded_rect(slider->bounds.x, track_y,
                           fill_width, track_height,
                           5, TOUCH_COLOR_PRIMARY);

    // Thumb
    int thumb_x = slider->bounds.x + fill_width - thumb_size/2;
    int thumb_y = slider->bounds.y + (slider->bounds.height - thumb_size) / 2;
    touch_draw_circle(thumb_x + thumb_size/2, thumb_y + thumb_size/2,
                     thumb_size/2, TOUCH_COLOR_PRIMARY);
}

void touch_slider_free(touch_slider_t* slider) {
    if (slider) free(slider);
}

// ============================================================================
// Widget API - Switch
// ============================================================================

touch_switch_t* touch_switch_create(int x, int y, const char* label) {
    touch_switch_t* sw = malloc(sizeof(touch_switch_t));
    if (!sw) return NULL;

    sw->bounds.x = x;
    sw->bounds.y = y;
    sw->bounds.width = 80;
    sw->bounds.height = 40;
    sw->state = false;
    sw->label = label ? strdup(label) : NULL;
    sw->on_toggle = NULL;
    sw->user_data = NULL;

    return sw;
}

void touch_switch_draw(touch_switch_t* sw) {
    if (!sw) return;

    uint32_t bg_color = sw->state ? TOUCH_COLOR_SUCCESS : TOUCH_COLOR_SECONDARY;

    // Background track
    touch_draw_rounded_rect(sw->bounds.x, sw->bounds.y,
                           sw->bounds.width, sw->bounds.height,
                           sw->bounds.height / 2, bg_color);

    // Thumb
    int thumb_size = sw->bounds.height - 8;
    int thumb_x = sw->state ?
                 sw->bounds.x + sw->bounds.width - thumb_size - 4 :
                 sw->bounds.x + 4;
    int thumb_y = sw->bounds.y + 4;

    touch_draw_circle(thumb_x + thumb_size/2, thumb_y + thumb_size/2,
                     thumb_size/2, TOUCH_COLOR_TEXT);

    // Label
    if (sw->label) {
        touch_draw_text(sw->bounds.x + sw->bounds.width + 20,
                       sw->bounds.y + 10,
                       sw->label, TOUCH_COLOR_TEXT, 20);
    }
}

void touch_switch_free(touch_switch_t* sw) {
    if (!sw) return;
    if (sw->label) free(sw->label);
    free(sw);
}

// ============================================================================
// Utilities
// ============================================================================

uint64_t touch_get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

void touch_sleep_ms(int ms) {
    usleep(ms * 1000);
}

bool touch_rect_contains(touch_rect_t* rect, int x, int y) {
    return x >= rect->x && x < rect->x + rect->width &&
           y >= rect->y && y < rect->y + rect->height;
}

bool touch_rect_intersects(touch_rect_t* a, touch_rect_t* b) {
    return !(a->x + a->width < b->x ||
             b->x + b->width < a->x ||
             a->y + a->height < b->y ||
             b->y + b->height < a->y);
}

uint32_t touch_color_alpha(uint32_t color, float alpha) {
    uint8_t a = (uint8_t)(((color >> 24) & 0xFF) * alpha);
    return (color & 0x00FFFFFF) | (a << 24);
}

uint32_t touch_color_blend(uint32_t color1, uint32_t color2, float t) {
    uint8_t r1 = (color1 >> 16) & 0xFF;
    uint8_t g1 = (color1 >> 8) & 0xFF;
    uint8_t b1 = color1 & 0xFF;

    uint8_t r2 = (color2 >> 16) & 0xFF;
    uint8_t g2 = (color2 >> 8) & 0xFF;
    uint8_t b2 = color2 & 0xFF;

    uint8_t r = (uint8_t)(r1 + (r2 - r1) * t);
    uint8_t g = (uint8_t)(g1 + (g2 - g1) * t);
    uint8_t b = (uint8_t)(b1 + (b2 - b1) * t);

    return 0xFF000000 | (r << 16) | (g << 8) | b;
}

// ============================================================================
// Haptic & Sound (Stubs)
// ============================================================================

void touch_haptic_light(void) {
    // Would interface with haptic hardware
}

void touch_haptic_medium(void) {
    // Would interface with haptic hardware
}

void touch_haptic_heavy(void) {
    // Would interface with haptic hardware
}

void touch_sound_tap(void) {
    // Would play tap sound
}

void touch_sound_success(void) {
    // Would play success sound
}

void touch_sound_error(void) {
    // Would play error sound
}

// ============================================================================
// Event Handling
// ============================================================================

void touch_register_event_handler(void (*handler)(touch_event_t* event)) {
    state.event_handler = handler;
}

void touch_inject_event(touch_event_t* event) {
    if (state.event_handler) {
        state.event_handler(event);
    }
}

// ============================================================================
// Main Loop
// ============================================================================

void touch_framework_run(void) {
    state.running = true;

    while (state.running) {
        // Read touch events
        // Process events
        // Render

        touch_sleep_ms(16);  // ~60 FPS
    }
}

void touch_framework_stop(void) {
    state.running = false;
}

// ============================================================================
// Keyboard API (Stubs)
// ============================================================================

void touch_keyboard_show(touch_text_input_t* target) {
    state.keyboard.visible = true;
    state.keyboard.target = target;
}

void touch_keyboard_hide(void) {
    state.keyboard.visible = false;
    state.keyboard.target = NULL;
}

void touch_keyboard_set_layout(osk_layout_t layout) {
    state.keyboard.layout = layout;
}

void touch_keyboard_draw(void) {
    if (!state.keyboard.visible) return;

    // Draw keyboard (implementation in apps for now)
}

void touch_keyboard_handle_touch(touch_event_t* event) {
    // Handle keyboard touches
}
