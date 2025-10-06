// userland/pkg-manager/tpkg-touch-gui.c
// TouchOS Package Manager - Touch-Optimized GUI
// Designed for Acer T230H touchscreen (1920x1080)
// Created by: floof<3

#include "tpkg.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Touch-friendly UI dimensions for 1920x1080
#define SCREEN_WIDTH  1920
#define SCREEN_HEIGHT 1080
#define BUTTON_HEIGHT 80
#define BUTTON_MARGIN 20
#define SIDEBAR_WIDTH 300
#define HEADER_HEIGHT 100
#define OSK_HEIGHT 350  // On-screen keyboard height

// UI Colors (ARGB)
#define COLOR_BG        0xFF1E1E1E
#define COLOR_SIDEBAR   0xFF2D2D30
#define COLOR_HEADER    0xFF007ACC
#define COLOR_BUTTON    0xFF3E3E42
#define COLOR_BUTTON_HOVER 0xFF505053
#define COLOR_TEXT      0xFFFFFFFF
#define COLOR_INSTALLED 0xFF4EC9B0
#define COLOR_AVAILABLE 0xFF569CD6

// UI States
typedef enum {
    VIEW_INSTALLED,
    VIEW_AVAILABLE,
    VIEW_SEARCH,
    VIEW_PACKAGE_INFO,
    VIEW_INSTALLING
} view_state_t;

// Touch button
typedef struct {
    int x, y, width, height;
    char label[64];
    void (*action)(void);
    bool enabled;
} touch_button_t;

// Package list item
typedef struct {
    char name[64];
    char version[32];
    char description[256];
    bool installed;
} package_item_t;

// UI State
static struct {
    view_state_t current_view;
    package_item_t packages[256];
    int package_count;
    int selected_package;
    int scroll_offset;
    bool osk_visible;
    char search_query[128];
    touch_button_t buttons[32];
    int button_count;
} ui_state = {0};

// Forward declarations
void draw_header(void);
void draw_sidebar(void);
void draw_package_list(void);
void draw_package_info(void);
void draw_on_screen_keyboard(void);
void handle_touch(int x, int y);
void refresh_package_list(void);
void install_selected_package(void);
void remove_selected_package(void);
void show_search_view(void);
void show_installed_view(void);
void show_available_view(void);
void show_package_info(void);

// ============================================================================
// Graphics Primitives (would use actual framebuffer in real implementation)
// ============================================================================

void fill_rect(int x, int y, int width, int height, uint32_t color) {
    // TODO: Use framebuffer to draw rectangle
    printf("DRAW_RECT: %d,%d %dx%d color=0x%08X\n", x, y, width, height, color);
}

void draw_text(int x, int y, const char* text, uint32_t color, int size) {
    // TODO: Use font rendering
    printf("DRAW_TEXT: %d,%d \"%s\" size=%d color=0x%08X\n", x, y, text, size, color);
}

void draw_rounded_rect(int x, int y, int width, int height, int radius, uint32_t color) {
    // TODO: Draw rounded rectangle
    fill_rect(x + radius, y, width - 2*radius, height, color);
    fill_rect(x, y + radius, width, height - 2*radius, color);
    // TODO: Draw corner circles
}

// ============================================================================
// Button Management
// ============================================================================

void add_button(int x, int y, int width, int height, const char* label, void (*action)(void)) {
    if (ui_state.button_count >= 32) return;

    touch_button_t* btn = &ui_state.buttons[ui_state.button_count++];
    btn->x = x;
    btn->y = y;
    btn->width = width;
    btn->height = height;
    strncpy(btn->label, label, sizeof(btn->label) - 1);
    btn->action = action;
    btn->enabled = true;
}

void clear_buttons(void) {
    ui_state.button_count = 0;
}

void draw_button(touch_button_t* btn) {
    uint32_t color = btn->enabled ? COLOR_BUTTON : 0xFF666666;
    draw_rounded_rect(btn->x, btn->y, btn->width, btn->height, 10, color);
    draw_text(btn->x + btn->width/2 - 50, btn->y + btn->height/2 - 10,
              btn->label, COLOR_TEXT, 24);
}

bool button_contains_point(touch_button_t* btn, int x, int y) {
    return x >= btn->x && x < btn->x + btn->width &&
           y >= btn->y && y < btn->y + btn->height;
}

// ============================================================================
// View: Header
// ============================================================================

void draw_header(void) {
    fill_rect(0, 0, SCREEN_WIDTH, HEADER_HEIGHT, COLOR_HEADER);
    draw_text(40, 35, "TouchOS Package Manager", COLOR_TEXT, 36);

    // Status info
    char status[128];
    snprintf(status, sizeof(status), "%d installed | %d available",
             ui_state.package_count, ui_state.package_count);
    draw_text(SCREEN_WIDTH - 400, 35, status, COLOR_TEXT, 20);
}

// ============================================================================
// View: Sidebar
// ============================================================================

void draw_sidebar(void) {
    fill_rect(0, HEADER_HEIGHT, SIDEBAR_WIDTH, SCREEN_HEIGHT - HEADER_HEIGHT, COLOR_SIDEBAR);

    int y = HEADER_HEIGHT + 40;

    // Navigation buttons
    clear_buttons();

    add_button(20, y, SIDEBAR_WIDTH - 40, BUTTON_HEIGHT, "📦 Installed", show_installed_view);
    y += BUTTON_HEIGHT + BUTTON_MARGIN;

    add_button(20, y, SIDEBAR_WIDTH - 40, BUTTON_HEIGHT, "🌐 Available", show_available_view);
    y += BUTTON_HEIGHT + BUTTON_MARGIN;

    add_button(20, y, SIDEBAR_WIDTH - 40, BUTTON_HEIGHT, "🔍 Search", show_search_view);
    y += BUTTON_HEIGHT + BUTTON_MARGIN;

    add_button(20, y, SIDEBAR_WIDTH - 40, BUTTON_HEIGHT, "🔄 Update", refresh_package_list);
    y += BUTTON_HEIGHT + BUTTON_MARGIN;

    // Draw buttons
    for (int i = 0; i < ui_state.button_count; i++) {
        draw_button(&ui_state.buttons[i]);
    }
}

// ============================================================================
// View: Package List
// ============================================================================

void draw_package_list(void) {
    int x = SIDEBAR_WIDTH + 40;
    int y = HEADER_HEIGHT + 40;
    int item_height = 120;
    int content_width = SCREEN_WIDTH - SIDEBAR_WIDTH - 80;

    // Title
    const char* title = ui_state.current_view == VIEW_INSTALLED ?
                       "Installed Packages" : "Available Packages";
    draw_text(x, y, title, COLOR_TEXT, 28);
    y += 60;

    // Package items (visible area)
    int visible_count = (SCREEN_HEIGHT - HEADER_HEIGHT - 100) / item_height;

    for (int i = ui_state.scroll_offset;
         i < ui_state.scroll_offset + visible_count && i < ui_state.package_count;
         i++) {
        package_item_t* pkg = &ui_state.packages[i];

        // Item background
        uint32_t bg_color = (i == ui_state.selected_package) ?
                           COLOR_BUTTON_HOVER : COLOR_BG;
        draw_rounded_rect(x, y, content_width, item_height - 10, 8, bg_color);

        // Package name (large, touch-friendly)
        draw_text(x + 20, y + 20, pkg->name, COLOR_TEXT, 28);

        // Version and status
        char info[128];
        snprintf(info, sizeof(info), "v%s %s",
                pkg->version, pkg->installed ? "✓ Installed" : "");
        draw_text(x + 20, y + 60, info,
                 pkg->installed ? COLOR_INSTALLED : COLOR_AVAILABLE, 20);

        // Description (truncated)
        char desc[100];
        strncpy(desc, pkg->description, 90);
        if (strlen(pkg->description) > 90) {
            strcpy(desc + 87, "...");
        }
        draw_text(x + 20, y + 90, desc, 0xFFAAAAAA, 16);

        y += item_height;
    }

    // Action buttons at bottom
    y = SCREEN_HEIGHT - OSK_HEIGHT - BUTTON_HEIGHT - 40;

    if (ui_state.selected_package >= 0) {
        package_item_t* pkg = &ui_state.packages[ui_state.selected_package];

        if (pkg->installed) {
            add_button(x, y, 200, BUTTON_HEIGHT, "🗑️ Remove", remove_selected_package);
        } else {
            add_button(x, y, 200, BUTTON_HEIGHT, "⬇️ Install", install_selected_package);
        }

        add_button(x + 220, y, 200, BUTTON_HEIGHT, "ℹ️ Info", show_package_info);
    }
}

// ============================================================================
// View: Package Info
// ============================================================================

void draw_package_info(void) {
    if (ui_state.selected_package < 0) return;

    package_item_t* pkg = &ui_state.packages[ui_state.selected_package];

    int x = SIDEBAR_WIDTH + 40;
    int y = HEADER_HEIGHT + 40;

    draw_text(x, y, pkg->name, COLOR_TEXT, 36);
    y += 60;

    char version[64];
    snprintf(version, sizeof(version), "Version: %s", pkg->version);
    draw_text(x, y, version, COLOR_TEXT, 24);
    y += 50;

    draw_text(x, y, "Description:", COLOR_TEXT, 20);
    y += 40;
    draw_text(x + 20, y, pkg->description, 0xFFCCCCCC, 18);
    y += 100;

    // TODO: Show dependencies, size, author, etc.

    // Back button
    y = SCREEN_HEIGHT - OSK_HEIGHT - BUTTON_HEIGHT - 40;
    add_button(x, y, 200, BUTTON_HEIGHT, "← Back", show_available_view);
}

// ============================================================================
// On-Screen Keyboard
// ============================================================================

void draw_on_screen_keyboard(void) {
    if (!ui_state.osk_visible) return;

    int y = SCREEN_HEIGHT - OSK_HEIGHT;
    fill_rect(0, y, SCREEN_WIDTH, OSK_HEIGHT, 0xFF3C3C3C);

    // Keyboard layout (optimized for touch)
    const char* rows[] = {
        "1234567890-=",
        "qwertyuiop",
        "asdfghjkl",
        "zxcvbnm"
    };

    int key_width = 140;
    int key_height = 70;
    int key_margin = 10;

    for (int row = 0; row < 4; row++) {
        int x = 50 + (row == 3 ? key_width : 0); // Indent bottom row
        y += 10;

        for (int col = 0; rows[row][col]; col++) {
            draw_rounded_rect(x, y, key_width - key_margin, key_height, 5, COLOR_BUTTON);

            char key_label[2] = {rows[row][col], 0};
            draw_text(x + key_width/2 - 10, y + key_height/2 - 10,
                     key_label, COLOR_TEXT, 28);

            x += key_width;
        }

        y += key_height + key_margin;
    }

    // Space bar
    int space_y = SCREEN_HEIGHT - 80;
    draw_rounded_rect(200, space_y, 1000, 60, 5, COLOR_BUTTON);
    draw_text(650, space_y + 20, "Space", COLOR_TEXT, 24);

    // Close keyboard button
    draw_rounded_rect(1600, space_y, 250, 60, 5, COLOR_HEADER);
    draw_text(1670, space_y + 20, "Close", COLOR_TEXT, 24);
}

// ============================================================================
// Touch Event Handlers
// ============================================================================

void handle_touch(int x, int y) {
    printf("Touch event: %d, %d\n", x, y);

    // Check buttons
    for (int i = 0; i < ui_state.button_count; i++) {
        touch_button_t* btn = &ui_state.buttons[i];
        if (btn->enabled && button_contains_point(btn, x, y)) {
            if (btn->action) {
                btn->action();
            }
            return;
        }
    }

    // Check package list items
    if (ui_state.current_view == VIEW_INSTALLED ||
        ui_state.current_view == VIEW_AVAILABLE) {
        int item_y = HEADER_HEIGHT + 100;
        int item_height = 120;

        if (x > SIDEBAR_WIDTH && y > item_y) {
            int index = ui_state.scroll_offset + (y - item_y) / item_height;
            if (index >= 0 && index < ui_state.package_count) {
                ui_state.selected_package = index;
                draw_package_list();
            }
        }
    }

    // Check on-screen keyboard
    if (ui_state.osk_visible) {
        // TODO: Handle keyboard input
    }
}

// ============================================================================
// Actions
// ============================================================================

void refresh_package_list(void) {
    printf("Refreshing package list...\n");
    tpkg_update_repo();

    // Reload installed packages
    tpkg_installed_t installed[256];
    ui_state.package_count = tpkg_list_installed(installed, 256);

    for (int i = 0; i < ui_state.package_count; i++) {
        strncpy(ui_state.packages[i].name, installed[i].metadata.name, 63);
        strncpy(ui_state.packages[i].version, installed[i].metadata.version, 31);
        strncpy(ui_state.packages[i].description, installed[i].metadata.description, 255);
        ui_state.packages[i].installed = true;
    }

    draw_package_list();
}

void install_selected_package(void) {
    if (ui_state.selected_package < 0) return;

    package_item_t* pkg = &ui_state.packages[ui_state.selected_package];

    printf("Installing package: %s\n", pkg->name);

    ui_state.current_view = VIEW_INSTALLING;
    draw_text(SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2,
              "Installing package...", COLOR_TEXT, 32);

    int result = tpkg_install(pkg->name);

    if (result == TPKG_OK) {
        printf("Package installed successfully!\n");
        pkg->installed = true;
    } else {
        printf("Installation failed: %s\n", tpkg_get_error_string(result));
    }

    ui_state.current_view = VIEW_AVAILABLE;
    draw_package_list();
}

void remove_selected_package(void) {
    if (ui_state.selected_package < 0) return;

    package_item_t* pkg = &ui_state.packages[ui_state.selected_package];

    printf("Removing package: %s\n", pkg->name);

    int result = tpkg_remove(pkg->name);

    if (result == TPKG_OK) {
        printf("Package removed successfully!\n");
        pkg->installed = false;
    } else {
        printf("Removal failed: %s\n", tpkg_get_error_string(result));
    }

    draw_package_list();
}

void show_search_view(void) {
    ui_state.current_view = VIEW_SEARCH;
    ui_state.osk_visible = true;
    draw_sidebar();
    draw_on_screen_keyboard();
}

void show_installed_view(void) {
    ui_state.current_view = VIEW_INSTALLED;
    ui_state.osk_visible = false;
    refresh_package_list();
}

void show_available_view(void) {
    ui_state.current_view = VIEW_AVAILABLE;
    ui_state.osk_visible = false;
    refresh_package_list();
}

void show_package_info(void) {
    ui_state.current_view = VIEW_PACKAGE_INFO;
    draw_header();
    draw_sidebar();
    draw_package_info();
}

// ============================================================================
// Main UI Loop
// ============================================================================

void ui_init(void) {
    tpkg_init();
    ui_state.current_view = VIEW_INSTALLED;
    ui_state.selected_package = -1;
    ui_state.scroll_offset = 0;
    refresh_package_list();
}

void ui_render(void) {
    // Clear screen
    fill_rect(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, COLOR_BG);

    // Draw UI components
    draw_header();
    draw_sidebar();

    switch (ui_state.current_view) {
        case VIEW_INSTALLED:
        case VIEW_AVAILABLE:
            draw_package_list();
            break;

        case VIEW_PACKAGE_INFO:
            draw_package_info();
            break;

        case VIEW_SEARCH:
            draw_package_list();
            draw_on_screen_keyboard();
            break;

        case VIEW_INSTALLING:
            draw_text(SCREEN_WIDTH/2 - 200, SCREEN_HEIGHT/2,
                     "Installing...", COLOR_TEXT, 32);
            break;
    }

    if (ui_state.osk_visible) {
        draw_on_screen_keyboard();
    }
}

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("TouchOS Package Manager - Touch GUI\n");
    printf("Optimized for Acer T230H (1920x1080)\n\n");

    ui_init();

    // Main event loop
    printf("Starting UI...\n");
    ui_render();

    // In real implementation, this would be an event loop
    // processing touch events from the USB touchscreen driver

    // Simulate some touch events for demo
    printf("\nSimulating touch events:\n");
    handle_touch(150, 200);  // Click "Installed" button
    handle_touch(800, 300);  // Select first package

    return 0;
}
