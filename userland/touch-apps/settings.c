// userland/touch-apps/settings.c
// TouchOS Settings - Touch-Optimized System Settings
// Created by: floof<3

#include "../libtouch/touch_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Settings categories
typedef struct {
    char* name;
    char* icon;
    void (*on_open)(void);
} settings_category_t;

// Settings state
typedef struct {
    int selected_category;
    int selected_option;

    // Display settings
    int brightness;
    bool night_mode;
    bool auto_brightness;

    // Touch settings
    int touch_sensitivity;  // 0-2 (low, medium, high)
    bool haptic_feedback;
    bool touch_sounds;

    // Network settings
    bool wifi_enabled;
    char wifi_ssid[64];
    bool bluetooth_enabled;

    // System settings
    bool auto_updates;
    int sleep_timeout;      // minutes
    char hostname[64];

} settings_state_t;

static settings_state_t settings = {0};

// ============================================================================
// Settings Pages
// ============================================================================

void draw_display_settings(void) {
    int y = 150;

    touch_draw_text(300, 100, "Display Settings", TOUCH_COLOR_TEXT, 32);

    // Brightness
    touch_draw_text(300, y, "☀️ Brightness", TOUCH_COLOR_TEXT, 24);
    y += 50;

    // Brightness slider
    touch_slider_t* brightness_slider = touch_slider_create(300, y, 800, 0, 100);
    brightness_slider->value = settings.brightness / 100.0f;
    touch_slider_draw(brightness_slider);

    char brightness_text[32];
    snprintf(brightness_text, sizeof(brightness_text), "%d%%", settings.brightness);
    touch_draw_text(1150, y, brightness_text, TOUCH_COLOR_TEXT_DIM, 20);
    y += 120;

    // Night mode
    touch_draw_text(300, y, "🌙 Night Mode", TOUCH_COLOR_TEXT, 24);
    touch_switch_t* night_switch = touch_switch_create(1100, y - 10, "");
    night_switch->state = settings.night_mode;
    touch_switch_draw(night_switch);
    y += 100;

    // Auto brightness
    touch_draw_text(300, y, "✨ Auto Brightness", TOUCH_COLOR_TEXT, 24);
    touch_switch_t* auto_bright = touch_switch_create(1100, y - 10, "");
    auto_bright->state = settings.auto_brightness;
    touch_switch_draw(auto_bright);
    y += 100;

    // Resolution info
    touch_draw_text(300, y, "Resolution: 1920×1080 (Full HD)", TOUCH_COLOR_TEXT_DIM, 20);
    y += 40;
    touch_draw_text(300, y, "Display: Acer T230H 23\"", TOUCH_COLOR_TEXT_DIM, 20);
}

void draw_touch_settings(void) {
    int y = 150;

    touch_draw_text(300, 100, "Touch Settings", TOUCH_COLOR_TEXT, 32);

    // Sensitivity
    touch_draw_text(300, y, "✋ Touch Sensitivity", TOUCH_COLOR_TEXT, 24);
    y += 50;

    // Sensitivity buttons
    const char* sensitivity_labels[] = {"Low", "Medium", "High"};
    for (int i = 0; i < 3; i++) {
        uint32_t color = (settings.touch_sensitivity == i) ?
                        TOUCH_COLOR_PRIMARY : TOUCH_COLOR_SECONDARY;

        touch_draw_rounded_rect(300 + i * 180, y, 160, TOUCH_BUTTON_HEIGHT,
                               10, color);
        touch_draw_text_centered(
            &(touch_rect_t){300 + i * 180, y, 160, TOUCH_BUTTON_HEIGHT},
            sensitivity_labels[i], TOUCH_COLOR_TEXT, 20);
    }
    y += 120;

    // Haptic feedback
    touch_draw_text(300, y, "📳 Haptic Feedback", TOUCH_COLOR_TEXT, 24);
    touch_draw_text(300, y + 30, "Feel vibrations when tapping", TOUCH_COLOR_TEXT_DIM, 18);
    touch_switch_t* haptic_switch = touch_switch_create(1100, y - 10, "");
    haptic_switch->state = settings.haptic_feedback;
    touch_switch_draw(haptic_switch);
    y += 120;

    // Touch sounds
    touch_draw_text(300, y, "🔊 Touch Sounds", TOUCH_COLOR_TEXT, 24);
    touch_draw_text(300, y + 30, "Play sounds when tapping buttons", TOUCH_COLOR_TEXT_DIM, 18);
    touch_switch_t* sound_switch = touch_switch_create(1100, y - 10, "");
    sound_switch->state = settings.touch_sounds;
    touch_switch_draw(sound_switch);
    y += 120;

    // Calibration
    touch_button_t* calibrate_btn = touch_button_create(
        300, y, 400, TOUCH_BUTTON_HEIGHT, "🎯 Calibrate Touchscreen");
    touch_button_draw(calibrate_btn);
    y += 100;

    // Touch info
    touch_draw_text(300, y, "Device: Acer T230H (0x0408:0x3000)", TOUCH_COLOR_TEXT_DIM, 20);
    y += 30;
    touch_draw_text(300, y, "Multi-touch: 2 points", TOUCH_COLOR_TEXT_DIM, 20);
    y += 30;
    touch_draw_text(300, y, "Calibration: 150,130 - 3946,3966", TOUCH_COLOR_TEXT_DIM, 20);
}

void draw_network_settings(void) {
    int y = 150;

    touch_draw_text(300, 100, "Network Settings", TOUCH_COLOR_TEXT, 32);

    // WiFi
    touch_draw_text(300, y, "📶 WiFi", TOUCH_COLOR_TEXT, 24);
    touch_switch_t* wifi_switch = touch_switch_create(1100, y - 10, "");
    wifi_switch->state = settings.wifi_enabled;
    touch_switch_draw(wifi_switch);
    y += 80;

    if (settings.wifi_enabled) {
        // Connected network
        touch_draw_rounded_rect(300, y, 800, 80, 10, TOUCH_COLOR_SURFACE);
        touch_draw_text(320, y + 15, "Connected to:", TOUCH_COLOR_TEXT_DIM, 18);
        touch_draw_text(320, y + 45, settings.wifi_ssid, TOUCH_COLOR_SUCCESS, 22);
        y += 100;

        // WiFi adapter info
        touch_draw_text(300, y, "Adapter: Intel Wireless-AC 8265", TOUCH_COLOR_TEXT_DIM, 18);
        y += 30;
        touch_draw_text(300, y, "Speed: 802.11ac (867 Mbps)", TOUCH_COLOR_TEXT_DIM, 18);
    }
    y += 80;

    // Bluetooth
    touch_draw_text(300, y, "📱 Bluetooth", TOUCH_COLOR_TEXT, 24);
    touch_switch_t* bt_switch = touch_switch_create(1100, y - 10, "");
    bt_switch->state = settings.bluetooth_enabled;
    touch_switch_draw(bt_switch);
    y += 80;

    if (settings.bluetooth_enabled) {
        touch_draw_text(300, y, "Bluetooth 4.2", TOUCH_COLOR_TEXT_DIM, 18);
    }
}

void draw_system_settings(void) {
    int y = 150;

    touch_draw_text(300, 100, "System Settings", TOUCH_COLOR_TEXT, 32);

    // Auto updates
    touch_draw_text(300, y, "🔄 Automatic Updates", TOUCH_COLOR_TEXT, 24);
    touch_draw_text(300, y + 30, "Download and install updates automatically", TOUCH_COLOR_TEXT_DIM, 18);
    touch_switch_t* update_switch = touch_switch_create(1100, y - 10, "");
    update_switch->state = settings.auto_updates;
    touch_switch_draw(update_switch);
    y += 120;

    // Sleep timeout
    touch_draw_text(300, y, "😴 Screen Sleep", TOUCH_COLOR_TEXT, 24);
    y += 50;

    const char* sleep_labels[] = {"1 min", "5 min", "15 min", "30 min", "Never"};
    int sleep_values[] = {1, 5, 15, 30, -1};
    for (int i = 0; i < 5; i++) {
        bool selected = (settings.sleep_timeout == sleep_values[i]);
        uint32_t color = selected ? TOUCH_COLOR_PRIMARY : TOUCH_COLOR_SECONDARY;

        touch_draw_rounded_rect(300 + i * 150, y, 140, 60, 10, color);
        touch_draw_text_centered(
            &(touch_rect_t){300 + i * 150, y, 140, 60},
            sleep_labels[i], TOUCH_COLOR_TEXT, 18);
    }
    y += 100;

    // Hostname
    touch_draw_text(300, y, "💻 Hostname", TOUCH_COLOR_TEXT, 24);
    y += 50;
    touch_draw_rounded_rect(300, y, 600, 60, 10, TOUCH_COLOR_SURFACE);
    touch_draw_text(320, y + 20, settings.hostname, TOUCH_COLOR_TEXT, 20);
    y += 100;

    // System info
    touch_draw_text(300, y, "ℹ️ System Information", TOUCH_COLOR_TEXT, 24);
    y += 50;

    touch_draw_text(320, y, "OS: TouchOS v1.0", TOUCH_COLOR_TEXT_DIM, 18);
    y += 30;
    touch_draw_text(320, y, "Kernel: 6.12.48+deb13-amd64", TOUCH_COLOR_TEXT_DIM, 18);
    y += 30;
    touch_draw_text(320, y, "CPU: Intel Core i5-8250U @ 1.6GHz", TOUCH_COLOR_TEXT_DIM, 18);
    y += 30;
    touch_draw_text(320, y, "RAM: 8GB LPDDR3", TOUCH_COLOR_TEXT_DIM, 18);
    y += 30;
    touch_draw_text(320, y, "Graphics: Intel UHD 620", TOUCH_COLOR_TEXT_DIM, 18);
}

void draw_about_settings(void) {
    int y = 200;

    touch_draw_text_centered(
        &(touch_rect_t){0, y, TOUCH_SCREEN_WIDTH, 50},
        "TouchOS", TOUCH_COLOR_PRIMARY, 48);
    y += 80;

    touch_draw_text_centered(
        &(touch_rect_t){0, y, TOUCH_SCREEN_WIDTH, 30},
        "Version 1.0", TOUCH_COLOR_TEXT, 24);
    y += 60;

    touch_draw_text_centered(
        &(touch_rect_t){0, y, TOUCH_SCREEN_WIDTH, 30},
        "An OS for the Masses", TOUCH_COLOR_TEXT_DIM, 20);
    y += 40;

    touch_draw_text_centered(
        &(touch_rect_t){0, y, TOUCH_SCREEN_WIDTH, 30},
        "(if the masses use touchscreen monitors)", TOUCH_COLOR_TEXT_DIM, 18);
    y += 100;

    touch_draw_emoji(TOUCH_SCREEN_WIDTH/2 - 50, y, "🖐️", 100);
    y += 120;

    touch_draw_text_centered(
        &(touch_rect_t){0, y, TOUCH_SCREEN_WIDTH, 30},
        "Custom Build", TOUCH_COLOR_TEXT_DIM, 20);
    y += 40;

    touch_draw_text_centered(
        &(touch_rect_t){0, y, TOUCH_SCREEN_WIDTH, 30},
        "Acer T230H + Dell Inspiron 13 7370", TOUCH_COLOR_TEXT_DIM, 18);
    y += 80;

    touch_draw_text_centered(
        &(touch_rect_t){0, y, TOUCH_SCREEN_WIDTH, 30},
        "Built with ❤️ by floof<3", TOUCH_COLOR_TEXT_DIM, 20);
}

// ============================================================================
// Main Settings UI
// ============================================================================

void draw_settings_sidebar(void) {
    // Sidebar background
    touch_draw_rect(0, 0, 280, TOUCH_SCREEN_HEIGHT, TOUCH_COLOR_SURFACE);

    // Title
    touch_draw_text(20, 20, "⚙️ Settings", TOUCH_COLOR_TEXT, 28);

    int y = 100;
    int item_height = 80;

    // Categories
    settings_category_t categories[] = {
        {"Display", "☀️", draw_display_settings},
        {"Touch", "✋", draw_touch_settings},
        {"Network", "📶", draw_network_settings},
        {"System", "💻", draw_system_settings},
        {"About", "ℹ️", draw_about_settings}
    };

    for (int i = 0; i < 5; i++) {
        bool selected = (i == settings.selected_category);
        uint32_t bg = selected ? TOUCH_COLOR_PRIMARY : TOUCH_COLOR_SURFACE;

        touch_draw_rounded_rect(10, y, 260, item_height - 10, 10, bg);

        char label[128];
        snprintf(label, sizeof(label), "%s  %s",
                categories[i].icon, categories[i].name);
        touch_draw_text(30, y + 25, label, TOUCH_COLOR_TEXT, 22);

        y += item_height;
    }
}

void render_settings(void) {
    // Background
    touch_clear_screen(TOUCH_COLOR_BG);

    // Sidebar
    draw_settings_sidebar();

    // Content area
    settings_category_t categories[] = {
        {"Display", "☀️", draw_display_settings},
        {"Touch", "✋", draw_touch_settings},
        {"Network", "📶", draw_network_settings},
        {"System", "💻", draw_system_settings},
        {"About", "ℹ️", draw_about_settings}
    };

    if (settings.selected_category >= 0 && settings.selected_category < 5) {
        categories[settings.selected_category].on_open();
    }

    // Back button
    touch_button_t* back_btn = touch_button_create(
        TOUCH_SCREEN_WIDTH - 220, TOUCH_SCREEN_HEIGHT - 100,
        200, TOUCH_BUTTON_HEIGHT, "← Back");
    touch_button_draw(back_btn);

    touch_flip_buffer();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("TouchOS Settings\n\n");

    // Initialize
    touch_framework_init();

    // Default settings
    settings.selected_category = 0;
    settings.brightness = 80;
    settings.night_mode = false;
    settings.auto_brightness = true;
    settings.touch_sensitivity = 1;  // Medium
    settings.haptic_feedback = true;
    settings.touch_sounds = true;
    settings.wifi_enabled = true;
    strcpy(settings.wifi_ssid, "MyNetwork");
    settings.bluetooth_enabled = false;
    settings.auto_updates = true;
    settings.sleep_timeout = 5;
    strcpy(settings.hostname, "touchos");

    // Main loop
    while (1) {
        render_settings();
        touch_sleep_ms(100);
    }

    touch_framework_shutdown();
    return 0;
}
