// wm/window_manager.c
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "../kernel/heap.h"

// Missing type definitions
typedef struct {
    int x, y, width, height;
} rect_t;

typedef struct {
    int x, y;
} touch_point_t;

typedef enum {
    TOUCH_DOWN,
    TOUCH_MOVE,
    TOUCH_UP
} touch_event_type_t;

typedef enum {
    INPUT_TYPE_TOUCHSCREEN,
    INPUT_TYPE_MOUSE
} input_type_t;

typedef enum {
    GESTURE_NONE,
    GESTURE_TAP,
    GESTURE_SWIPE,
    GESTURE_PINCH
} gesture_type_t;

typedef enum {
    EV_ABS,
    EV_KEY
} event_type_t;

typedef enum {
    ABS_MT_SLOT,
    ABS_MT_TRACKING_ID,
    ABS_MT_POSITION_X,
    ABS_MT_POSITION_Y,
    ABS_X,
    ABS_Y
} abs_code_t;

typedef enum {
    BTN_TOUCH,
    KEY_SPACE
} key_code_t;

typedef struct {
    event_type_t type;
    uint32_t code;
    int value;
} input_event_t;

typedef struct {
    rect_t bounds;
    char label;
    key_code_t keycode;
    bool is_pressed;
} key_button_t;

// Forward declarations
static inline bool rect_contains(rect_t* r, int x, int y) {
    return x >= r->x && x < r->x + r->width &&
           y >= r->y && y < r->y + r->height;
}

static inline int abs(int x) { return x < 0 ? -x : x; }

void input_register_handler(input_type_t type, void (*handler)(input_event_t*));
void input_report_event(input_event_t* event);
void input_sync(void);
uint64_t get_system_time(void);
void compositor_damage_region(int x, int y, int width, int height);
void framebuffer_fill_rect(int x, int y, int width, int height, uint32_t color);
void framebuffer_fill_rounded_rect(int x, int y, int width, int height, int radius, uint32_t color);
void framebuffer_draw_rounded_rect(int x, int y, int width, int height, int radius, uint32_t color);
void font_draw_string(const char* str, int x, int y, uint32_t color);
key_code_t osk_char_to_keycode(char c);

// Stub for framebuffer access
static struct { uint32_t width, height; } fb = { 1920, 1080 };

typedef struct window {
    int id;
    rect_t bounds;
    char* title;
    
    // Window properties
    uint32_t flags;
    uint32_t* buffer;
    
    // Touch interaction state
    struct {
        bool is_being_dragged;
        int drag_offset_x;
        int drag_offset_y;
        int resize_edge;  // Which edge is being resized
        uint64_t last_tap_time;  // For double-tap detection
    } touch_state;
    
    // On-screen keyboard association
    bool needs_keyboard;
    rect_t keyboard_avoid_rect;
    
    // Window decorations
    int titlebar_height;
    int border_width;
    
    struct window* next;
    struct window* prev;
} window_t;

typedef struct {
    window_t* windows;
    window_t* focused_window;
    
    // Touch gesture recognition
    struct {
        int active_touches;
        touch_point_t touches[10];
        uint64_t gesture_start_time;
        gesture_type_t current_gesture;
    } gesture_state;
    
    // On-screen keyboard
    struct {
        bool visible;
        window_t* target_window;
        rect_t bounds;
        key_button_t* keys;
        int key_count;
        int current_layout;  // QWERTY, numeric, symbols
    } osk;
    
} window_manager_t;

static window_manager_t wm = {0};

// Forward declarations for WM functions
void wm_handle_touch(input_event_t* event);
void wm_handle_mouse(input_event_t* event);
void wm_handle_touch_down(int slot);
void wm_handle_touch_up(int slot);
void wm_handle_touch_move(int slot);
void osk_init(void);
void osk_handle_touch(int x, int y);
void osk_show_for_window(window_t* win);
void osk_render(void);
window_t* wm_window_at_point(int x, int y);
void wm_focus_window(window_t* win);
bool wm_is_on_resize_edge(window_t* win, int x, int y);
int wm_get_resize_edge(window_t* win, int x, int y);
void wm_toggle_maximize(window_t* win);
void wm_send_touch_to_window(window_t* win, int x, int y, touch_event_type_t type);
bool wm_is_text_input_at(window_t* win, int x, int y);
void wm_handle_pinch_gesture(void);
void wm_resize_window_edge(window_t* win, int x, int y, int edge);

void wm_init(void) {
    wm.windows = NULL;
    wm.focused_window = NULL;
    
    // Initialize on-screen keyboard
    osk_init();
    
    // Register input handlers
    input_register_handler(INPUT_TYPE_TOUCHSCREEN, wm_handle_touch);
    input_register_handler(INPUT_TYPE_MOUSE, wm_handle_mouse);
}

void wm_handle_touch(input_event_t* event) {
    if (event->type == EV_ABS) {
        if (event->code == ABS_MT_SLOT) {
            wm.gesture_state.active_touches = event->value + 1;
        } else if (event->code == ABS_MT_TRACKING_ID) {
            int slot = wm.gesture_state.active_touches - 1;
            if (event->value == -1) {
                // Touch released
                wm_handle_touch_up(slot);
            } else {
                // New touch
                wm_handle_touch_down(slot);
            }
        } else if (event->code == ABS_MT_POSITION_X) {
            int slot = wm.gesture_state.active_touches - 1;
            wm.gesture_state.touches[slot].x = event->value;
        } else if (event->code == ABS_MT_POSITION_Y) {
            int slot = wm.gesture_state.active_touches - 1;
            wm.gesture_state.touches[slot].y = event->value;
            wm_handle_touch_move(slot);
        }
    }
}

void wm_handle_mouse(input_event_t* event) {
    (void)event;
    // TODO: Implement mouse handling
}

void wm_handle_touch_up(int slot) {
    (void)slot;
    // TODO: Implement touch up handling
}

void osk_handle_touch(int x, int y) {
    (void)x; (void)y;
    // TODO: Implement keyboard touch handling
}

void wm_handle_touch_down(int slot) {
    touch_point_t* touch = &wm.gesture_state.touches[slot];
    
    // Check if touch is on on-screen keyboard
    if (wm.osk.visible && rect_contains(&wm.osk.bounds, touch->x, touch->y)) {
        osk_handle_touch(touch->x - wm.osk.bounds.x, 
                        touch->y - wm.osk.bounds.y);
        return;
    }
    
    // Find window under touch
    window_t* win = wm_window_at_point(touch->x, touch->y);
    if (!win) return;
    
    // Focus window
    wm_focus_window(win);
    
    // Check if touch is on window decorations
    if (touch->y < win->bounds.y + win->titlebar_height) {
        // Titlebar - start drag
        win->touch_state.is_being_dragged = true;
        win->touch_state.drag_offset_x = touch->x - win->bounds.x;
        win->touch_state.drag_offset_y = touch->y - win->bounds.y;
        
        // Check for double-tap to maximize
        uint64_t now = get_system_time();
        if (now - win->touch_state.last_tap_time < 500000) {  // 500ms
            wm_toggle_maximize(win);
        }
        win->touch_state.last_tap_time = now;
        
    } else if (wm_is_on_resize_edge(win, touch->x, touch->y)) {
        // Window edge - start resize
        win->touch_state.resize_edge = wm_get_resize_edge(win, touch->x, touch->y);
        
    } else {
        // Client area - send to application
        wm_send_touch_to_window(win, touch->x - win->bounds.x,
                               touch->y - win->bounds.y - win->titlebar_height,
                               TOUCH_DOWN);
        
        // Check if this is a text input area
        if (wm_is_text_input_at(win, touch->x - win->bounds.x,
                               touch->y - win->bounds.y - win->titlebar_height)) {
            osk_show_for_window(win);
        }
    }
}

void wm_handle_touch_move(int slot) {
    touch_point_t* touch = &wm.gesture_state.touches[slot];
    
    // Handle multitouch gestures
    if (wm.gesture_state.active_touches == 2) {
        wm_handle_pinch_gesture();
        return;
    }
    
    // Single touch handling
    window_t* win = wm.focused_window;
    if (!win) return;
    
    if (win->touch_state.is_being_dragged) {
        // Move window
        win->bounds.x = touch->x - win->touch_state.drag_offset_x;
        win->bounds.y = touch->y - win->touch_state.drag_offset_y;
        
        // Snap to screen edges
        if (abs(win->bounds.x) < 10) win->bounds.x = 0;
        if (abs(win->bounds.y) < 10) win->bounds.y = 0;
        if (abs(win->bounds.x + win->bounds.width - fb.width) < 10) {
            win->bounds.x = fb.width - win->bounds.width;
        }
        
        compositor_damage_region(win->bounds.x, win->bounds.y,
                               win->bounds.width, win->bounds.height);
                               
    } else if (win->touch_state.resize_edge) {
        wm_resize_window_edge(win, touch->x, touch->y, win->touch_state.resize_edge);
        
    } else {
        // Send to application
        wm_send_touch_to_window(win, touch->x - win->bounds.x,
                               touch->y - win->bounds.y - win->titlebar_height,
                               TOUCH_MOVE);
    }
}

// On-screen keyboard implementation
void osk_init(void) {
    wm.osk.bounds = (rect_t){
        .x = 0,
        .y = fb.height - 300,  // Keyboard height
        .width = fb.width,
        .height = 300
    };
    
    // Create QWERTY layout
    wm.osk.keys = kmalloc(sizeof(key_button_t) * 50);
    
    const char* rows[] = {
        "1234567890",
        "qwertyuiop",
        "asdfghjkl",
        "zxcvbnm"
    };
    
    int key_width = fb.width / 11;
    int key_height = 60;
    int y_offset = 10;
    
    int key_index = 0;
    for (int row = 0; row < 4; row++) {
        int x_offset = (row == 3) ? key_width : 10;  // Indent bottom row
        
        for (int col = 0; rows[row][col]; col++) {
            wm.osk.keys[key_index++] = (key_button_t){
                .bounds = {
                    .x = x_offset + col * key_width,
                    .y = y_offset + row * (key_height + 10),
                    .width = key_width - 5,
                    .height = key_height
                },
                .label = rows[row][col],
                .keycode = osk_char_to_keycode(rows[row][col]),
                .is_pressed = false
            };
        }
    }
    
    // Add special keys (space, backspace, enter, shift)
    wm.osk.keys[key_index++] = (key_button_t){
        .bounds = {.x = 10, .y = 250, .width = fb.width - 20, .height = 40},
        .label = ' ',
        .keycode = KEY_SPACE
    };
    
    wm.osk.key_count = key_index;
    wm.osk.visible = false;
}

void osk_show_for_window(window_t* win) {
    wm.osk.visible = true;
    wm.osk.target_window = win;
    
    // Adjust window position if it would be covered by keyboard
    if (win->bounds.y + win->bounds.height > wm.osk.bounds.y) {
        int new_y = wm.osk.bounds.y - win->bounds.height - 10;
        if (new_y < 0) {
            // Window too tall, resize it
            win->bounds.height = wm.osk.bounds.y - 10;
            new_y = 0;
        }
        win->bounds.y = new_y;
    }
    
    compositor_damage_region(0, 0, fb.width, fb.height);
}

void osk_render(void) {
    if (!wm.osk.visible) return;
    
    // Draw keyboard background
    framebuffer_fill_rect(wm.osk.bounds.x, wm.osk.bounds.y,
                          wm.osk.bounds.width, wm.osk.bounds.height,
                          0xFFE0E0E0);
    
    // Draw keys
    for (int i = 0; i < wm.osk.key_count; i++) {
        key_button_t* key = &wm.osk.keys[i];
        
        uint32_t color = key->is_pressed ? 0xFFB0B0B0 : 0xFFFFFFFF;
        
        // Draw key background with rounded corners
        framebuffer_fill_rounded_rect(
            wm.osk.bounds.x + key->bounds.x,
            wm.osk.bounds.y + key->bounds.y,
            key->bounds.width, key->bounds.height,
            5, color);
        
        // Draw key border
        framebuffer_draw_rounded_rect(
            wm.osk.bounds.x + key->bounds.x,
            wm.osk.bounds.y + key->bounds.y,
            key->bounds.width, key->bounds.height,
            5, 0xFF808080);
        
        // Draw key label
        if (key->label != ' ') {
            char label_str[2] = {key->label, 0};
            font_draw_string(label_str,
                           wm.osk.bounds.x + key->bounds.x + key->bounds.width/2 - 8,
                           wm.osk.bounds.y + key->bounds.y + key->bounds.height/2 - 10,
                           0xFF000000);
        }
    }
}
// Stub implementations for missing functions
void input_register_handler(input_type_t type, void (*handler)(input_event_t*)) {
    (void)type; (void)handler;
    // TODO: Implement input handler registration
}

void input_report_event(input_event_t* event) {
    (void)event;
    // TODO: Implement event reporting
}

void input_sync(void) {
    // TODO: Implement sync
}

uint64_t get_system_time(void) {
    // TODO: Implement system time
    return 0;
}

void compositor_damage_region(int x, int y, int width, int height) {
    (void)x; (void)y; (void)width; (void)height;
    // TODO: Implement damage tracking
}

void framebuffer_fill_rect(int x, int y, int width, int height, uint32_t color) {
    (void)x; (void)y; (void)width; (void)height; (void)color;
    // TODO: Implement rectangle fill
}

void framebuffer_fill_rounded_rect(int x, int y, int width, int height, int radius, uint32_t color) {
    (void)x; (void)y; (void)width; (void)height; (void)radius; (void)color;
    // TODO: Implement rounded rectangle fill
}

void framebuffer_draw_rounded_rect(int x, int y, int width, int height, int radius, uint32_t color) {
    (void)x; (void)y; (void)width; (void)height; (void)radius; (void)color;
    // TODO: Implement rounded rectangle draw
}

void font_draw_string(const char* str, int x, int y, uint32_t color) {
    (void)str; (void)x; (void)y; (void)color;
    // TODO: Implement text rendering
}

key_code_t osk_char_to_keycode(char c) {
    (void)c;
    // TODO: Implement character to keycode conversion
    return KEY_SPACE;
}

window_t* wm_window_at_point(int x, int y) {
    (void)x; (void)y;
    return NULL;
}

void wm_focus_window(window_t* win) {
    (void)win;
}

bool wm_is_on_resize_edge(window_t* win, int x, int y) {
    (void)win; (void)x; (void)y;
    return false;
}

int wm_get_resize_edge(window_t* win, int x, int y) {
    (void)win; (void)x; (void)y;
    return 0;
}

void wm_toggle_maximize(window_t* win) {
    (void)win;
}

void wm_send_touch_to_window(window_t* win, int x, int y, touch_event_type_t type) {
    (void)win; (void)x; (void)y; (void)type;
}

bool wm_is_text_input_at(window_t* win, int x, int y) {
    (void)win; (void)x; (void)y;
    return false;
}

void wm_handle_pinch_gesture(void) {
}

void wm_resize_window_edge(window_t* win, int x, int y, int edge) {
    (void)win; (void)x; (void)y; (void)edge;
}

// Rev1 Of the Window manager
// Floof<3
