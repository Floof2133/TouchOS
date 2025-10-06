// userland/libtouch/touch_framework.h
// TouchOS Unified Touch Framework
// Single API for all touch applications
// Created by: floof<3

#ifndef TOUCH_FRAMEWORK_H
#define TOUCH_FRAMEWORK_H

#include <stdint.h>
#include <stdbool.h>

// ============================================================================
// Touch Framework Constants
// ============================================================================

// Screen dimensions (Acer T230H)
#define TOUCH_SCREEN_WIDTH  1920
#define TOUCH_SCREEN_HEIGHT 1080

// Touch-optimized sizes (based on ergonomics research)
#define TOUCH_MIN_TARGET    44      // Minimum touch target (Apple HIG)
#define TOUCH_BUTTON_HEIGHT 80      // Standard button height
#define TOUCH_BUTTON_WIDTH  200     // Standard button width
#define TOUCH_MARGIN        20      // Standard margin between elements
#define TOUCH_CORNER_RADIUS 10      // Rounded corners for easier tapping
#define TOUCH_ICON_SIZE     48      // Icon size
#define TOUCH_FONT_SIZE     24      // Default font size

// Keyboard
#define TOUCH_OSK_HEIGHT    350     // On-screen keyboard height
#define TOUCH_KEY_WIDTH     140     // Keyboard key width
#define TOUCH_KEY_HEIGHT    70      // Keyboard key height

// Timing (milliseconds)
#define TOUCH_TAP_TIMEOUT       200     // Max time for tap
#define TOUCH_LONG_PRESS        500     // Min time for long press
#define TOUCH_DOUBLE_TAP        300     // Max time between taps
#define TOUCH_SWIPE_MIN_DIST    50      // Min distance for swipe
#define TOUCH_ANIMATION_SPEED   200     // Animation duration

// Colors (ARGB)
#define TOUCH_COLOR_BG              0xFF1E1E1E
#define TOUCH_COLOR_SURFACE         0xFF2D2D30
#define TOUCH_COLOR_PRIMARY         0xFF007ACC
#define TOUCH_COLOR_SECONDARY       0xFF3E3E42
#define TOUCH_COLOR_TEXT            0xFFFFFFFF
#define TOUCH_COLOR_TEXT_DIM        0xFFAAAAAA
#define TOUCH_COLOR_SUCCESS         0xFF4EC9B0
#define TOUCH_COLOR_WARNING         0xFFFFC107
#define TOUCH_COLOR_ERROR           0xFFF44336
#define TOUCH_COLOR_HOVER           0xFF505053
#define TOUCH_COLOR_PRESSED         0xFF606063

// ============================================================================
// Core Touch Types
// ============================================================================

// Touch point
typedef struct {
    int x, y;
    int id;                 // Touch ID for multi-touch tracking
    bool active;
    uint64_t timestamp;
} touch_point_t;

// Touch event types
typedef enum {
    TOUCH_EVENT_DOWN,       // Finger down
    TOUCH_EVENT_MOVE,       // Finger moving
    TOUCH_EVENT_UP,         // Finger up
    TOUCH_EVENT_TAP,        // Quick tap
    TOUCH_EVENT_LONG_PRESS, // Long press
    TOUCH_EVENT_DOUBLE_TAP, // Double tap
    TOUCH_EVENT_SWIPE_LEFT,
    TOUCH_EVENT_SWIPE_RIGHT,
    TOUCH_EVENT_SWIPE_UP,
    TOUCH_EVENT_SWIPE_DOWN,
    TOUCH_EVENT_PINCH_IN,   // Zoom out
    TOUCH_EVENT_PINCH_OUT,  // Zoom in
    TOUCH_EVENT_ROTATE
} touch_event_type_t;

// Touch event
typedef struct {
    touch_event_type_t type;
    touch_point_t points[10];   // Up to 10 touch points
    int point_count;
    int x, y;                   // Primary point
    int dx, dy;                 // Delta for movement
    float scale;                // For pinch gesture
    float rotation;             // For rotate gesture
    uint64_t timestamp;
} touch_event_t;

// ============================================================================
// UI Widgets
// ============================================================================

// Rectangle
typedef struct {
    int x, y, width, height;
} touch_rect_t;

// Button
typedef struct {
    touch_rect_t bounds;
    char* label;
    char* icon;             // Icon name or emoji
    uint32_t color;
    bool enabled;
    bool visible;
    void (*on_tap)(void* user_data);
    void* user_data;
    int id;
} touch_button_t;

// List item
typedef struct {
    char* title;
    char* subtitle;
    char* icon;
    bool selected;
    void* user_data;
} touch_list_item_t;

// List view
typedef struct {
    touch_rect_t bounds;
    touch_list_item_t* items;
    int item_count;
    int selected_index;
    int scroll_offset;
    int item_height;
    void (*on_select)(int index, void* user_data);
    void* user_data;
} touch_list_view_t;

// Text input
typedef struct {
    touch_rect_t bounds;
    char* text;
    int max_length;
    char* placeholder;
    bool focused;
    bool password;
    void (*on_change)(const char* text, void* user_data);
    void* user_data;
} touch_text_input_t;

// Slider
typedef struct {
    touch_rect_t bounds;
    float value;            // 0.0 to 1.0
    float min, max;
    void (*on_change)(float value, void* user_data);
    void* user_data;
} touch_slider_t;

// Switch/Toggle
typedef struct {
    touch_rect_t bounds;
    bool state;
    char* label;
    void (*on_toggle)(bool state, void* user_data);
    void* user_data;
} touch_switch_t;

// ============================================================================
// Layout Types
// ============================================================================

typedef enum {
    LAYOUT_VERTICAL,
    LAYOUT_HORIZONTAL,
    LAYOUT_GRID
} touch_layout_type_t;

typedef struct {
    touch_layout_type_t type;
    touch_rect_t bounds;
    int spacing;
    int padding;
    int columns;            // For grid layout
} touch_layout_t;

// ============================================================================
// Window/App Types
// ============================================================================

typedef struct touch_window {
    char* title;
    touch_rect_t bounds;
    uint32_t bg_color;
    bool fullscreen;
    bool has_titlebar;
    bool can_resize;

    // Widgets
    touch_button_t* buttons;
    int button_count;

    // Callbacks
    void (*on_render)(struct touch_window* win);
    void (*on_touch)(struct touch_window* win, touch_event_t* event);
    void (*on_close)(struct touch_window* win);

    void* user_data;
    int id;
} touch_window_t;

// App info
typedef struct {
    char* name;
    char* icon;
    char* description;
    void (*on_launch)(void);
    touch_window_t* window;
} touch_app_t;

// ============================================================================
// On-Screen Keyboard
// ============================================================================

typedef enum {
    OSK_LAYOUT_QWERTY,
    OSK_LAYOUT_NUMERIC,
    OSK_LAYOUT_SYMBOLS
} osk_layout_t;

typedef struct {
    bool visible;
    osk_layout_t layout;
    touch_rect_t bounds;
    touch_text_input_t* target;
    void (*on_key)(char key);
    void (*on_submit)(const char* text);
} touch_keyboard_t;

// ============================================================================
// Framework API
// ============================================================================

// Initialization
void touch_framework_init(void);
void touch_framework_shutdown(void);

// Main loop
void touch_framework_run(void);
void touch_framework_stop(void);

// Event handling
void touch_register_event_handler(void (*handler)(touch_event_t* event));
void touch_inject_event(touch_event_t* event);

// ============================================================================
// Drawing API
// ============================================================================

// Basic shapes
void touch_draw_rect(int x, int y, int w, int h, uint32_t color);
void touch_draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color);
void touch_draw_circle(int x, int y, int radius, uint32_t color);
void touch_draw_line(int x1, int y1, int x2, int y2, uint32_t color, int thickness);

// Text
void touch_draw_text(int x, int y, const char* text, uint32_t color, int size);
void touch_draw_text_centered(touch_rect_t* rect, const char* text, uint32_t color, int size);
int touch_measure_text(const char* text, int size);

// Images/Icons
void touch_draw_icon(int x, int y, const char* icon_name, int size, uint32_t color);
void touch_draw_emoji(int x, int y, const char* emoji, int size);

// Effects
void touch_draw_shadow(touch_rect_t* rect, int blur, uint32_t color);
void touch_draw_gradient(touch_rect_t* rect, uint32_t color1, uint32_t color2, bool vertical);

// Clipping
void touch_set_clip(touch_rect_t* rect);
void touch_clear_clip(void);

// Screen
void touch_clear_screen(uint32_t color);
void touch_flip_buffer(void);

// ============================================================================
// Widget API
// ============================================================================

// Button
touch_button_t* touch_button_create(int x, int y, int w, int h, const char* label);
void touch_button_set_icon(touch_button_t* btn, const char* icon);
void touch_button_draw(touch_button_t* btn);
bool touch_button_hit_test(touch_button_t* btn, int x, int y);
void touch_button_free(touch_button_t* btn);

// List
touch_list_view_t* touch_list_create(int x, int y, int w, int h);
void touch_list_add_item(touch_list_view_t* list, const char* title, const char* subtitle, const char* icon);
void touch_list_draw(touch_list_view_t* list);
void touch_list_handle_touch(touch_list_view_t* list, touch_event_t* event);
void touch_list_free(touch_list_view_t* list);

// Text input
touch_text_input_t* touch_input_create(int x, int y, int w, const char* placeholder);
void touch_input_draw(touch_text_input_t* input);
void touch_input_handle_key(touch_text_input_t* input, char key);
void touch_input_free(touch_text_input_t* input);

// Slider
touch_slider_t* touch_slider_create(int x, int y, int w, float min, float max);
void touch_slider_draw(touch_slider_t* slider);
void touch_slider_handle_touch(touch_slider_t* slider, touch_event_t* event);
void touch_slider_free(touch_slider_t* slider);

// Switch
touch_switch_t* touch_switch_create(int x, int y, const char* label);
void touch_switch_draw(touch_switch_t* sw);
void touch_switch_handle_touch(touch_switch_t* sw, touch_event_t* event);
void touch_switch_free(touch_switch_t* sw);

// ============================================================================
// Window API
// ============================================================================

touch_window_t* touch_window_create(const char* title, int w, int h);
void touch_window_set_fullscreen(touch_window_t* win, bool fullscreen);
void touch_window_add_button(touch_window_t* win, touch_button_t* btn);
void touch_window_show(touch_window_t* win);
void touch_window_hide(touch_window_t* win);
void touch_window_close(touch_window_t* win);
void touch_window_render(touch_window_t* win);

// ============================================================================
// Keyboard API
// ============================================================================

void touch_keyboard_show(touch_text_input_t* target);
void touch_keyboard_hide(void);
void touch_keyboard_set_layout(osk_layout_t layout);
void touch_keyboard_draw(void);
void touch_keyboard_handle_touch(touch_event_t* event);

// ============================================================================
// Gesture Recognition
// ============================================================================

void touch_gesture_update(touch_event_t* event);
bool touch_gesture_is_tap(touch_event_t* event);
bool touch_gesture_is_long_press(touch_event_t* event);
bool touch_gesture_is_swipe(touch_event_t* event, int* direction);
bool touch_gesture_is_pinch(touch_event_t* event, float* scale);

// ============================================================================
// App Management
// ============================================================================

void touch_app_register(touch_app_t* app);
void touch_app_launch(const char* name);
void touch_app_close(const char* name);
touch_app_t** touch_app_get_all(int* count);

// ============================================================================
// Utilities
// ============================================================================

// Timing
uint64_t touch_get_time_ms(void);
void touch_sleep_ms(int ms);

// Rectangle helpers
bool touch_rect_contains(touch_rect_t* rect, int x, int y);
bool touch_rect_intersects(touch_rect_t* a, touch_rect_t* b);

// Color helpers
uint32_t touch_color_alpha(uint32_t color, float alpha);
uint32_t touch_color_blend(uint32_t color1, uint32_t color2, float t);

// Haptic feedback (if supported)
void touch_haptic_light(void);
void touch_haptic_medium(void);
void touch_haptic_heavy(void);

// Sound feedback
void touch_sound_tap(void);
void touch_sound_success(void);
void touch_sound_error(void);

#endif // TOUCH_FRAMEWORK_H
