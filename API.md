# TouchOS libtouch API Reference

← [Back to README](README.md) | [Drivers](DRIVERS.md) | [Next: Building](BUILDING.md) →

## Overview

**libtouch** is the core Touch UI framework for TouchOS. It provides a complete API for building touch-optimized applications.

**Header**: `userland/libtouch/touch_framework.h`
**Library**: `userland/libtouch/libtouch.a`
**Language**: C

## Quick Start

```c
#include "../libtouch/touch_framework.h"

int main() {
    // Initialize framework
    touch_framework_init();
    
    // Create UI
    touch_button_t* btn = touch_button_create(100, 100, 200, 80, "Hello!");
    btn->color = TOUCH_COLOR_PRIMARY;
    
    // Main loop
    while (running) {
        touch_clear_screen(TOUCH_COLOR_BG);
        touch_button_draw(btn);
        touch_flip_buffer();
    }
    
    touch_framework_shutdown();
    return 0;
}
```

**Compile**:
```bash
gcc myapp.c ../libtouch/libtouch.a -o myapp
```

## Core API

### Framework Lifecycle

```c
void touch_framework_init(void);      // Initialize (call first)
void touch_framework_shutdown(void);  // Cleanup (call last)
void touch_framework_run(void);       // Start event loop
void touch_framework_stop(void);      // Stop event loop
```

### Event Handling

```c
// Register event handler
void touch_register_event_handler(void (*handler)(touch_event_t* event));

// Event types
typedef enum {
    TOUCH_EVENT_TAP,
    TOUCH_EVENT_LONG_PRESS,
    TOUCH_EVENT_SWIPE_LEFT/RIGHT/UP/DOWN,
    TOUCH_EVENT_PINCH_IN/OUT,
    TOUCH_EVENT_ROTATE
} touch_event_type_t;

// Event structure
typedef struct {
    touch_event_type_t type;
    int x, y;                 // Touch location
    touch_point_t points[10]; // Multi-touch (up to 10)
    float scale;              // Pinch scale
    float rotation;           // Rotation angle
} touch_event_t;
```

## Drawing API

### Basic Shapes

```c
void touch_clear_screen(uint32_t color);
void touch_draw_rect(int x, int y, int w, int h, uint32_t color);
void touch_draw_rounded_rect(int x, int y, int w, int h, int radius, uint32_t color);
void touch_draw_circle(int x, int y, int radius, uint32_t color);
void touch_draw_line(int x1, int y1, int x2, int y2, uint32_t color, int thickness);
```

### Text

```c
void touch_draw_text(int x, int y, const char* text, uint32_t color, int size);
void touch_draw_text_centered(touch_rect_t* rect, const char* text, uint32_t color, int size);
int touch_measure_text(const char* text, int size);
```

### Screen

```c
void touch_flip_buffer(void);  // Swap front/back buffers
```

## Widget API

### Buttons

```c
// Create button
touch_button_t* touch_button_create(int x, int y, int w, int h, const char* label);

// Configure
void touch_button_set_icon(touch_button_t* btn, const char* icon);
btn->color = TOUCH_COLOR_PRIMARY;
btn->on_tap = my_callback;

// Draw
void touch_button_draw(touch_button_t* btn);

// Hit test
bool touch_button_hit_test(touch_button_t* btn, int x, int y);

// Cleanup
void touch_button_free(touch_button_t* btn);
```

### Lists

```c
touch_list_view_t* touch_list_create(int x, int y, int w, int h);
void touch_list_add_item(touch_list_view_t* list, const char* title, const char* subtitle, const char* icon);
void touch_list_draw(touch_list_view_t* list);
void touch_list_handle_touch(touch_list_view_t* list, touch_event_t* event);
```

### Text Input

```c
touch_text_input_t* touch_input_create(int x, int y, int w, const char* placeholder);
void touch_input_draw(touch_text_input_t* input);
void touch_input_handle_key(touch_text_input_t* input, char key);
```

### Sliders

```c
touch_slider_t* touch_slider_create(int x, int y, int w, float min, float max);
void touch_slider_draw(touch_slider_t* slider);
void touch_slider_handle_touch(touch_slider_t* slider, touch_event_t* event);
```

## Design Constants

```c
// Screen
#define TOUCH_SCREEN_WIDTH  1920
#define TOUCH_SCREEN_HEIGHT 1080

// Touch targets
#define TOUCH_MIN_TARGET    44   // Minimum touch size
#define TOUCH_BUTTON_HEIGHT 80   // Standard button height
#define TOUCH_BUTTON_WIDTH  200  // Standard button width

// Colors (ARGB)
#define TOUCH_COLOR_BG              0xFF1E1E1E
#define TOUCH_COLOR_PRIMARY         0xFF007ACC
#define TOUCH_COLOR_SUCCESS         0xFF4EC9B0
#define TOUCH_COLOR_WARNING         0xFFFFC107
#define TOUCH_COLOR_ERROR           0xFFF44336
```

## Best Practices

1. **Touch Targets**: Minimum 44×44 pixels
2. **Button Spacing**: 20px margins
3. **Feedback**: Provide visual/haptic feedback
4. **Gestures**: Support both tap and swipe
5. **Text Size**: Minimum 24px for readability

## Example Applications

See `userland/touch-apps/` for complete examples.

← [Back to README](README.md) | [Drivers](DRIVERS.md) | [Next: Building](BUILDING.md) →
