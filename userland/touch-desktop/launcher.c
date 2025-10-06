// userland/touch-desktop/launcher.c
// TouchOS Desktop Launcher - Touch-First Home Screen
// Designed for Acer T230H (1920x1080)
// Created by: floof<3

#include "../libtouch/touch_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// ============================================================================
// Desktop Layout
// ============================================================================

// Status bar (top)
#define STATUS_BAR_HEIGHT 60

// App grid
#define APP_ICON_SIZE 120
#define APP_ICON_SPACING 40
#define APP_LABEL_HEIGHT 40
#define APP_COLS 8
#define APP_ROWS 4

// Dock (bottom)
#define DOCK_HEIGHT 140
#define DOCK_ICON_SIZE 100

// Quick settings panel (swipe from right)
#define QUICK_SETTINGS_WIDTH 400

// ============================================================================
// Desktop State
// ============================================================================

typedef struct {
    char* name;
    char* icon;
    char* exec;
    int x, y;
} desktop_app_t;

typedef struct {
    // Time & Date
    char time_str[32];
    char date_str[64];

    // Status
    int battery;        // -1 = AC only
    bool wifi_connected;
    int cpu_usage;
    int ram_usage;

    // Apps
    desktop_app_t apps[32];
    int app_count;
    int selected_app;

    // Dock apps
    desktop_app_t dock_apps[6];
    int dock_count;

    // UI State
    bool quick_settings_open;
    int notification_count;
    bool osk_visible;

    // Wallpaper
    uint32_t wallpaper_color;

} desktop_state_t;

static desktop_state_t desktop = {0};

// ============================================================================
// App Definitions
// ============================================================================

void register_builtin_apps(void) {
    // Package Manager
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Packages",
        .icon = "📦",
        .exec = "/usr/bin/tpkg-touch-gui"
    };

    // Settings
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Settings",
        .icon = "⚙️",
        .exec = "/usr/bin/touch-settings"
    };

    // File Manager
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Files",
        .icon = "📁",
        .exec = "/usr/bin/touch-files"
    };

    // Terminal
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Terminal",
        .icon = "💻",
        .exec = "/usr/bin/touch-terminal"
    };

    // Browser
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Browser",
        .icon = "🌐",
        .exec = "/usr/bin/touch-browser"
    };

    // System Monitor
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Monitor",
        .icon = "📊",
        .exec = "/usr/bin/touch-monitor"
    };

    // Calculator
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Calculator",
        .icon = "🔢",
        .exec = "/usr/bin/touch-calc"
    };

    // Text Editor
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Editor",
        .icon = "📝",
        .exec = "/usr/bin/touch-editor"
    };

    // Photos
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Photos",
        .icon = "🖼️",
        .exec = "/usr/bin/touch-photos"
    };

    // Music
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Music",
        .icon = "🎵",
        .exec = "/usr/bin/touch-music"
    };

    // Videos
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Videos",
        .icon = "🎬",
        .exec = "/usr/bin/touch-videos"
    };

    // Calendar
    desktop.apps[desktop.app_count++] = (desktop_app_t){
        .name = "Calendar",
        .icon = "📅",
        .exec = "/usr/bin/touch-calendar"
    };

    // Calculate app positions
    int start_x = (TOUCH_SCREEN_WIDTH - (APP_COLS * (APP_ICON_SIZE + APP_ICON_SPACING))) / 2;
    int start_y = STATUS_BAR_HEIGHT + 60;

    for (int i = 0; i < desktop.app_count; i++) {
        int row = i / APP_COLS;
        int col = i % APP_COLS;
        desktop.apps[i].x = start_x + col * (APP_ICON_SIZE + APP_ICON_SPACING);
        desktop.apps[i].y = start_y + row * (APP_ICON_SIZE + APP_LABEL_HEIGHT + APP_ICON_SPACING);
    }

    // Dock apps (most used)
    desktop.dock_apps[0] = (desktop_app_t){.name = "Files", .icon = "📁", .exec = "/usr/bin/touch-files"};
    desktop.dock_apps[1] = (desktop_app_t){.name = "Browser", .icon = "🌐", .exec = "/usr/bin/touch-browser"};
    desktop.dock_apps[2] = (desktop_app_t){.name = "Terminal", .icon = "💻", .exec = "/usr/bin/touch-terminal"};
    desktop.dock_apps[3] = (desktop_app_t){.name = "Packages", .icon = "📦", .exec = "/usr/bin/tpkg-touch-gui"};
    desktop.dock_apps[4] = (desktop_app_t){.name = "Settings", .icon = "⚙️", .exec = "/usr/bin/touch-settings"};
    desktop.dock_count = 5;
}

// ============================================================================
// Status Bar
// ============================================================================

void update_status(void) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    // Update time
    snprintf(desktop.time_str, sizeof(desktop.time_str),
             "%02d:%02d", t->tm_hour, t->tm_min);

    // Update date
    const char* days[] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char* months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
                           "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    snprintf(desktop.date_str, sizeof(desktop.date_str),
             "%s, %s %d", days[t->tm_wday], months[t->tm_mon], t->tm_mday);

    // Get system stats
    FILE* fp = fopen("/proc/stat", "r");
    if (fp) {
        // Parse CPU usage
        fclose(fp);
    }
    desktop.cpu_usage = 25;  // Placeholder
    desktop.ram_usage = 45;  // Placeholder

    desktop.wifi_connected = true;  // Placeholder
    desktop.battery = -1;  // AC only
}

void draw_status_bar(void) {
    // Background
    touch_draw_rect(0, 0, TOUCH_SCREEN_WIDTH, STATUS_BAR_HEIGHT,
                   TOUCH_COLOR_SURFACE);

    // Time (left)
    touch_draw_text(20, 20, desktop.time_str, TOUCH_COLOR_TEXT, 28);

    // Date (left of center)
    touch_draw_text(120, 20, desktop.date_str, TOUCH_COLOR_TEXT_DIM, 20);

    // Status icons (right side)
    int x = TOUCH_SCREEN_WIDTH - 20;

    // WiFi
    if (desktop.wifi_connected) {
        touch_draw_emoji(x - 50, 15, "📶", 32);
        x -= 60;
    }

    // CPU usage
    char cpu_text[16];
    snprintf(cpu_text, sizeof(cpu_text), "CPU %d%%", desktop.cpu_usage);
    touch_draw_text(x - 100, 20, cpu_text, TOUCH_COLOR_TEXT_DIM, 18);
    x -= 120;

    // RAM usage
    char ram_text[16];
    snprintf(ram_text, sizeof(ram_text), "RAM %d%%", desktop.ram_usage);
    touch_draw_text(x - 100, 20, ram_text, TOUCH_COLOR_TEXT_DIM, 18);
    x -= 120;

    // Notifications
    if (desktop.notification_count > 0) {
        char notif[16];
        snprintf(notif, sizeof(notif), "🔔 %d", desktop.notification_count);
        touch_draw_text(x - 80, 20, notif, TOUCH_COLOR_WARNING, 18);
    }
}

// ============================================================================
// App Grid
// ============================================================================

void draw_app_grid(void) {
    for (int i = 0; i < desktop.app_count; i++) {
        desktop_app_t* app = &desktop.apps[i];

        // Icon background
        bool is_selected = (i == desktop.selected_app);
        uint32_t bg_color = is_selected ? TOUCH_COLOR_PRIMARY : TOUCH_COLOR_SURFACE;

        touch_draw_rounded_rect(app->x, app->y, APP_ICON_SIZE, APP_ICON_SIZE,
                               15, bg_color);

        // Icon (emoji)
        touch_draw_emoji(app->x + APP_ICON_SIZE/2 - 30, app->y + 20,
                        app->icon, 60);

        // Label
        int label_y = app->y + APP_ICON_SIZE + 10;
        touch_draw_text_centered(
            &(touch_rect_t){app->x, label_y, APP_ICON_SIZE, APP_LABEL_HEIGHT},
            app->name, TOUCH_COLOR_TEXT, 18);
    }
}

// ============================================================================
// Dock
// ============================================================================

void draw_dock(void) {
    int dock_y = TOUCH_SCREEN_HEIGHT - DOCK_HEIGHT;

    // Dock background with blur effect
    touch_draw_rounded_rect(
        (TOUCH_SCREEN_WIDTH - (desktop.dock_count * (DOCK_ICON_SIZE + 30) + 30)) / 2,
        dock_y + 10,
        desktop.dock_count * (DOCK_ICON_SIZE + 30) + 30,
        DOCK_HEIGHT - 20,
        20,
        touch_color_alpha(TOUCH_COLOR_SURFACE, 0.9));

    // Dock icons
    int x = (TOUCH_SCREEN_WIDTH - (desktop.dock_count * (DOCK_ICON_SIZE + 30))) / 2;

    for (int i = 0; i < desktop.dock_count; i++) {
        desktop_app_t* app = &desktop.dock_apps[i];

        // Icon background
        touch_draw_rounded_rect(x, dock_y + 20, DOCK_ICON_SIZE, DOCK_ICON_SIZE,
                               15, TOUCH_COLOR_BG);

        // Icon
        touch_draw_emoji(x + DOCK_ICON_SIZE/2 - 35, dock_y + 30,
                        app->icon, 70);

        x += DOCK_ICON_SIZE + 30;
    }
}

// ============================================================================
// Quick Settings Panel
// ============================================================================

void draw_quick_settings(void) {
    if (!desktop.quick_settings_open) return;

    int x = TOUCH_SCREEN_WIDTH - QUICK_SETTINGS_WIDTH;

    // Background
    touch_draw_rect(x, 0, QUICK_SETTINGS_WIDTH, TOUCH_SCREEN_HEIGHT,
                   TOUCH_COLOR_SURFACE);

    // Title
    touch_draw_text(x + 30, 30, "Quick Settings", TOUCH_COLOR_TEXT, 28);

    int y = 100;

    // WiFi toggle
    touch_draw_text(x + 30, y, "📶 WiFi", TOUCH_COLOR_TEXT, 24);
    touch_draw_rounded_rect(x + QUICK_SETTINGS_WIDTH - 90, y - 5, 60, 34, 17,
                           desktop.wifi_connected ? TOUCH_COLOR_SUCCESS : TOUCH_COLOR_SECONDARY);
    y += 70;

    // Brightness slider
    touch_draw_text(x + 30, y, "☀️ Brightness", TOUCH_COLOR_TEXT, 24);
    y += 40;
    touch_draw_rounded_rect(x + 30, y, QUICK_SETTINGS_WIDTH - 60, 10, 5,
                           TOUCH_COLOR_SECONDARY);
    touch_draw_rounded_rect(x + 30, y - 5, 200, 20, 10, TOUCH_COLOR_PRIMARY); // Thumb
    y += 70;

    // Volume slider
    touch_draw_text(x + 30, y, "🔊 Volume", TOUCH_COLOR_TEXT, 24);
    y += 40;
    touch_draw_rounded_rect(x + 30, y, QUICK_SETTINGS_WIDTH - 60, 10, 5,
                           TOUCH_COLOR_SECONDARY);
    touch_draw_rounded_rect(x + 30, y - 5, 150, 20, 10, TOUCH_COLOR_PRIMARY);
    y += 70;

    // Night mode toggle
    touch_draw_text(x + 30, y, "🌙 Night Mode", TOUCH_COLOR_TEXT, 24);
    touch_draw_rounded_rect(x + QUICK_SETTINGS_WIDTH - 90, y - 5, 60, 34, 17,
                           TOUCH_COLOR_SECONDARY);
    y += 70;

    // Rotation lock
    touch_draw_text(x + 30, y, "🔒 Rotation Lock", TOUCH_COLOR_TEXT, 24);
    touch_draw_rounded_rect(x + QUICK_SETTINGS_WIDTH - 90, y - 5, 60, 34, 17,
                           TOUCH_COLOR_SECONDARY);
    y += 100;

    // System info
    touch_draw_text(x + 30, y, "System Info", TOUCH_COLOR_TEXT_DIM, 20);
    y += 40;

    char info[128];
    snprintf(info, sizeof(info), "CPU: %d%%", desktop.cpu_usage);
    touch_draw_text(x + 30, y, info, TOUCH_COLOR_TEXT_DIM, 18);
    y += 30;

    snprintf(info, sizeof(info), "RAM: %d%%", desktop.ram_usage);
    touch_draw_text(x + 30, y, info, TOUCH_COLOR_TEXT_DIM, 18);
    y += 30;

    snprintf(info, sizeof(info), "TouchOS v1.0");
    touch_draw_text(x + 30, y, info, TOUCH_COLOR_TEXT_DIM, 18);
}

// ============================================================================
// Touch Event Handling
// ============================================================================

void handle_touch_event(touch_event_t* event) {
    // Status bar tap - open quick settings
    if (event->type == TOUCH_EVENT_TAP && event->y < STATUS_BAR_HEIGHT) {
        desktop.quick_settings_open = !desktop.quick_settings_open;
        return;
    }

    // Swipe from right edge - quick settings
    if (event->type == TOUCH_EVENT_SWIPE_LEFT &&
        event->x > TOUCH_SCREEN_WIDTH - 50) {
        desktop.quick_settings_open = true;
        return;
    }

    // Swipe right on quick settings - close
    if (event->type == TOUCH_EVENT_SWIPE_RIGHT &&
        desktop.quick_settings_open) {
        desktop.quick_settings_open = false;
        return;
    }

    // Close quick settings if tapping outside
    if (desktop.quick_settings_open &&
        event->x < TOUCH_SCREEN_WIDTH - QUICK_SETTINGS_WIDTH) {
        desktop.quick_settings_open = false;
        return;
    }

    // App grid taps
    for (int i = 0; i < desktop.app_count; i++) {
        desktop_app_t* app = &desktop.apps[i];

        if (event->x >= app->x && event->x < app->x + APP_ICON_SIZE &&
            event->y >= app->y && event->y < app->y + APP_ICON_SIZE) {

            if (event->type == TOUCH_EVENT_TAP) {
                desktop.selected_app = i;
                touch_sound_tap();
            } else if (event->type == TOUCH_EVENT_DOUBLE_TAP) {
                // Launch app
                printf("Launching: %s (%s)\n", app->name, app->exec);
                touch_sound_success();

                // Fork and exec in real implementation
                // For now, just print
                system(app->exec);
            }
            return;
        }
    }

    // Dock taps
    int dock_y = TOUCH_SCREEN_HEIGHT - DOCK_HEIGHT;
    int dock_x = (TOUCH_SCREEN_WIDTH - (desktop.dock_count * (DOCK_ICON_SIZE + 30))) / 2;

    for (int i = 0; i < desktop.dock_count; i++) {
        int x = dock_x + i * (DOCK_ICON_SIZE + 30);

        if (event->type == TOUCH_EVENT_TAP &&
            event->x >= x && event->x < x + DOCK_ICON_SIZE &&
            event->y >= dock_y + 20 && event->y < dock_y + 20 + DOCK_ICON_SIZE) {

            desktop_app_t* app = &desktop.dock_apps[i];
            printf("Launching from dock: %s\n", app->name);
            touch_sound_tap();
            system(app->exec);
            return;
        }
    }
}

// ============================================================================
// Main Loop
// ============================================================================

void render_desktop(void) {
    // Wallpaper
    touch_clear_screen(desktop.wallpaper_color);

    // Draw components
    draw_app_grid();
    draw_dock();
    draw_status_bar();

    // Overlay
    if (desktop.quick_settings_open) {
        draw_quick_settings();
    }

    // Flip buffer
    touch_flip_buffer();
}

void desktop_loop(void) {
    while (1) {
        // Update status
        update_status();

        // Render
        render_desktop();

        // Handle events (would be event-driven in real implementation)
        touch_sleep_ms(100);
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("TouchOS Desktop Launcher\n");
    printf("Optimized for Acer T230H (1920x1080)\n\n");

    // Initialize framework
    touch_framework_init();

    // Setup desktop
    desktop.wallpaper_color = 0xFF0A0A0A;  // Very dark gray
    desktop.selected_app = -1;
    desktop.quick_settings_open = false;
    desktop.notification_count = 0;

    // Register apps
    register_builtin_apps();

    // Register touch handler
    touch_register_event_handler(handle_touch_event);

    // Update initial status
    update_status();

    printf("Desktop ready! %d apps registered.\n", desktop.app_count);
    printf("Dock has %d apps.\n\n", desktop.dock_count);

    // Run desktop
    desktop_loop();

    touch_framework_shutdown();
    return 0;
}
