// userland/touch-apps/file-manager.c
// TouchOS File Manager - Touch-Optimized File Browser
// Created by: floof<3

#include "../libtouch/touch_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

// File manager constants
#define MAX_FILES 1000
#define MAX_PATH 512
#define ICON_SIZE 100
#define ICON_SPACING 40
#define TOOLBAR_HEIGHT 100
#define BREADCRUMB_HEIGHT 80
#define SIDEBAR_WIDTH 320

// File types
typedef enum {
    FILE_TYPE_FOLDER,
    FILE_TYPE_TEXT,
    FILE_TYPE_IMAGE,
    FILE_TYPE_VIDEO,
    FILE_TYPE_AUDIO,
    FILE_TYPE_ARCHIVE,
    FILE_TYPE_EXECUTABLE,
    FILE_TYPE_DOCUMENT,
    FILE_TYPE_OTHER
} file_type_t;

// File entry
typedef struct {
    char name[256];
    char path[MAX_PATH];
    file_type_t type;
    size_t size;
    time_t modified;
    bool selected;
    bool is_directory;
} file_entry_t;

// File manager state
typedef struct {
    char current_path[MAX_PATH];
    file_entry_t files[MAX_FILES];
    int file_count;
    int scroll_offset;
    int selected_index;
    bool multi_select_mode;

    // View settings
    bool show_hidden;
    bool grid_view;  // true = grid, false = list

    // Navigation history
    char path_history[20][MAX_PATH];
    int history_index;
    int history_count;

    // Clipboard
    char clipboard_paths[100][MAX_PATH];
    int clipboard_count;
    bool clipboard_is_cut;  // true = cut, false = copy

    // UI state
    bool show_context_menu;
    int context_menu_x;
    int context_menu_y;
    int context_file_index;

} file_manager_t;

static file_manager_t fm = {0};

// ============================================================================
// File Type Detection
// ============================================================================

const char* get_file_extension(const char* filename) {
    const char* dot = strrchr(filename, '.');
    if (!dot || dot == filename) return "";
    return dot + 1;
}

file_type_t detect_file_type(const char* path, bool is_dir) {
    if (is_dir) return FILE_TYPE_FOLDER;

    const char* ext = get_file_extension(path);

    // Text files
    if (strcmp(ext, "txt") == 0 || strcmp(ext, "md") == 0 ||
        strcmp(ext, "c") == 0 || strcmp(ext, "h") == 0 ||
        strcmp(ext, "cpp") == 0 || strcmp(ext, "sh") == 0) {
        return FILE_TYPE_TEXT;
    }

    // Images
    if (strcmp(ext, "jpg") == 0 || strcmp(ext, "jpeg") == 0 ||
        strcmp(ext, "png") == 0 || strcmp(ext, "gif") == 0 ||
        strcmp(ext, "bmp") == 0 || strcmp(ext, "svg") == 0) {
        return FILE_TYPE_IMAGE;
    }

    // Videos
    if (strcmp(ext, "mp4") == 0 || strcmp(ext, "avi") == 0 ||
        strcmp(ext, "mkv") == 0 || strcmp(ext, "mov") == 0) {
        return FILE_TYPE_VIDEO;
    }

    // Audio
    if (strcmp(ext, "mp3") == 0 || strcmp(ext, "wav") == 0 ||
        strcmp(ext, "flac") == 0 || strcmp(ext, "ogg") == 0) {
        return FILE_TYPE_AUDIO;
    }

    // Archives
    if (strcmp(ext, "zip") == 0 || strcmp(ext, "tar") == 0 ||
        strcmp(ext, "gz") == 0 || strcmp(ext, "tpkg") == 0) {
        return FILE_TYPE_ARCHIVE;
    }

    // Documents
    if (strcmp(ext, "pdf") == 0 || strcmp(ext, "doc") == 0 ||
        strcmp(ext, "docx") == 0 || strcmp(ext, "odt") == 0) {
        return FILE_TYPE_DOCUMENT;
    }

    return FILE_TYPE_OTHER;
}

const char* get_file_icon(file_type_t type) {
    switch (type) {
        case FILE_TYPE_FOLDER:      return "📁";
        case FILE_TYPE_TEXT:        return "📄";
        case FILE_TYPE_IMAGE:       return "🖼️";
        case FILE_TYPE_VIDEO:       return "🎬";
        case FILE_TYPE_AUDIO:       return "🎵";
        case FILE_TYPE_ARCHIVE:     return "📦";
        case FILE_TYPE_EXECUTABLE:  return "⚙️";
        case FILE_TYPE_DOCUMENT:    return "📝";
        default:                    return "📄";
    }
}

// ============================================================================
// File Operations
// ============================================================================

void load_directory(const char* path) {
    DIR* dir = opendir(path);
    if (!dir) {
        printf("Failed to open directory: %s\n", path);
        return;
    }

    fm.file_count = 0;
    struct dirent* entry;

    while ((entry = readdir(dir)) != NULL && fm.file_count < MAX_FILES) {
        // Skip current directory
        if (strcmp(entry->d_name, ".") == 0) continue;

        // Skip hidden files unless enabled
        if (entry->d_name[0] == '.' && !fm.show_hidden) continue;

        file_entry_t* file = &fm.files[fm.file_count];

        strncpy(file->name, entry->d_name, sizeof(file->name) - 1);
        snprintf(file->path, sizeof(file->path), "%s/%s", path, entry->d_name);

        struct stat st;
        if (stat(file->path, &st) == 0) {
            file->is_directory = S_ISDIR(st.st_mode);
            file->size = st.st_size;
            file->modified = st.st_mtime;
            file->type = detect_file_type(file->path, file->is_directory);
        }

        file->selected = false;
        fm.file_count++;
    }

    closedir(dir);

    // Sort: directories first, then alphabetically
    for (int i = 0; i < fm.file_count - 1; i++) {
        for (int j = i + 1; j < fm.file_count; j++) {
            bool swap = false;

            if (fm.files[i].is_directory && !fm.files[j].is_directory) {
                continue;
            } else if (!fm.files[i].is_directory && fm.files[j].is_directory) {
                swap = true;
            } else if (strcmp(fm.files[i].name, fm.files[j].name) > 0) {
                swap = true;
            }

            if (swap) {
                file_entry_t temp = fm.files[i];
                fm.files[i] = fm.files[j];
                fm.files[j] = temp;
            }
        }
    }

    strncpy(fm.current_path, path, sizeof(fm.current_path) - 1);
    fm.scroll_offset = 0;
    fm.selected_index = -1;
}

void navigate_to(const char* path) {
    // Save to history
    if (fm.history_index < 19) {
        strncpy(fm.path_history[fm.history_index], fm.current_path, MAX_PATH - 1);
        fm.history_index++;
        fm.history_count = fm.history_index;
    }

    load_directory(path);
}

void navigate_back(void) {
    if (fm.history_index > 0) {
        fm.history_index--;
        load_directory(fm.path_history[fm.history_index]);
    }
}

void navigate_up(void) {
    // Navigate to parent directory
    char parent[MAX_PATH];
    strncpy(parent, fm.current_path, sizeof(parent) - 1);

    char* last_slash = strrchr(parent, '/');
    if (last_slash && last_slash != parent) {
        *last_slash = '\0';
        navigate_to(parent);
    } else if (strcmp(parent, "/") != 0) {
        navigate_to("/");
    }
}

// ============================================================================
// UI Drawing
// ============================================================================

void draw_breadcrumb_nav(void) {
    int y = 0;
    touch_draw_rect(0, y, TOUCH_SCREEN_WIDTH, BREADCRUMB_HEIGHT, TOUCH_COLOR_SURFACE);

    // Back button
    touch_button_t* back_btn = touch_button_create(20, 15, 120, 50, "← Back");
    touch_button_draw(back_btn);

    // Up button
    touch_button_t* up_btn = touch_button_create(160, 15, 100, 50, "↑ Up");
    touch_button_draw(up_btn);

    // Path breadcrumbs
    int x = 280;
    char path_copy[MAX_PATH];
    strncpy(path_copy, fm.current_path, sizeof(path_copy) - 1);

    char* token = strtok(path_copy, "/");
    char partial_path[MAX_PATH] = "/";

    // Root
    touch_draw_text(x, 30, "/", TOUCH_COLOR_TEXT_DIM, 24);
    x += 30;

    while (token != NULL && x < TOUCH_SCREEN_WIDTH - 200) {
        if (strlen(partial_path) > 1) {
            strncat(partial_path, "/", MAX_PATH - strlen(partial_path) - 1);
        }
        strncat(partial_path, token, MAX_PATH - strlen(partial_path) - 1);

        touch_draw_text(x, 30, ">", TOUCH_COLOR_TEXT_DIM, 20);
        x += 25;

        touch_draw_text(x, 30, token, TOUCH_COLOR_PRIMARY, 22);
        x += touch_measure_text(token, 22) + 15;

        token = strtok(NULL, "/");
    }
}

void draw_sidebar(void) {
    int x = 0;
    int y = BREADCRUMB_HEIGHT;
    int sidebar_height = TOUCH_SCREEN_HEIGHT - BREADCRUMB_HEIGHT - TOOLBAR_HEIGHT;

    touch_draw_rect(x, y, SIDEBAR_WIDTH, sidebar_height, TOUCH_COLOR_SURFACE);

    // Sidebar title
    touch_draw_text(x + 20, y + 20, "📍 Locations", TOUCH_COLOR_TEXT, 24);

    y += 70;
    int item_height = 70;

    // Common locations
    const char* locations[] = {
        "🏠 Home",
        "📄 Documents",
        "⬇️ Downloads",
        "🖼️ Pictures",
        "🎵 Music",
        "🎬 Videos",
        "💾 System"
    };

    for (int i = 0; i < 7; i++) {
        uint32_t bg = TOUCH_COLOR_SURFACE;

        touch_draw_rounded_rect(x + 10, y, SIDEBAR_WIDTH - 20, item_height - 10, 10, bg);
        touch_draw_text(x + 30, y + 20, locations[i], TOUCH_COLOR_TEXT, 20);

        y += item_height;
    }

    // View options
    y = TOUCH_SCREEN_HEIGHT - TOOLBAR_HEIGHT - 200;
    touch_draw_text(x + 20, y, "⚙️ View", TOUCH_COLOR_TEXT, 20);
    y += 50;

    // Grid/List toggle
    const char* view_mode = fm.grid_view ? "Grid View ✓" : "List View ✓";
    touch_draw_text(x + 30, y, view_mode, TOUCH_COLOR_TEXT_DIM, 18);
    y += 40;

    // Show hidden
    const char* hidden_text = fm.show_hidden ? "Show Hidden ✓" : "Show Hidden";
    touch_draw_text(x + 30, y, hidden_text, TOUCH_COLOR_TEXT_DIM, 18);
}

void draw_file_grid(void) {
    int start_x = SIDEBAR_WIDTH + 40;
    int start_y = BREADCRUMB_HEIGHT + 40;
    int cols = (TOUCH_SCREEN_WIDTH - SIDEBAR_WIDTH - 80) / (ICON_SIZE + ICON_SPACING);

    int x = start_x;
    int y = start_y - fm.scroll_offset;

    for (int i = 0; i < fm.file_count; i++) {
        file_entry_t* file = &fm.files[i];

        // Skip if scrolled out of view
        if (y + ICON_SIZE + 60 < BREADCRUMB_HEIGHT ||
            y > TOUCH_SCREEN_HEIGHT - TOOLBAR_HEIGHT) {
            goto next_position;
        }

        // Selection background
        if (file->selected || i == fm.selected_index) {
            touch_draw_rounded_rect(x - 10, y - 10, ICON_SIZE + 20, ICON_SIZE + 80,
                                   10, TOUCH_COLOR_PRIMARY);
        }

        // Icon
        touch_draw_emoji(x, y, get_file_icon(file->type), ICON_SIZE);

        // Filename (truncated)
        char display_name[32];
        strncpy(display_name, file->name, sizeof(display_name) - 1);
        if (strlen(file->name) > 15) {
            display_name[12] = '.';
            display_name[13] = '.';
            display_name[14] = '.';
            display_name[15] = '\0';
        }

        int text_width = touch_measure_text(display_name, 18);
        int text_x = x + (ICON_SIZE - text_width) / 2;
        touch_draw_text(text_x, y + ICON_SIZE + 10, display_name, TOUCH_COLOR_TEXT, 18);

        // Multi-select checkbox
        if (fm.multi_select_mode) {
            uint32_t checkbox_color = file->selected ? TOUCH_COLOR_SUCCESS : TOUCH_COLOR_SECONDARY;
            touch_draw_rounded_rect(x + ICON_SIZE - 30, y, 25, 25, 5, checkbox_color);
            if (file->selected) {
                touch_draw_text(x + ICON_SIZE - 27, y + 2, "✓", TOUCH_COLOR_TEXT, 18);
            }
        }

next_position:
        x += ICON_SIZE + ICON_SPACING;
        if (x + ICON_SIZE > TOUCH_SCREEN_WIDTH - 40) {
            x = start_x;
            y += ICON_SIZE + 80;
        }
    }
}

void draw_file_list(void) {
    int x = SIDEBAR_WIDTH + 20;
    int y = BREADCRUMB_HEIGHT + 20;
    int item_height = 90;

    for (int i = 0; i < fm.file_count; i++) {
        file_entry_t* file = &fm.files[i];

        // Skip if scrolled out of view
        int item_y = y + i * item_height - fm.scroll_offset;
        if (item_y + item_height < BREADCRUMB_HEIGHT ||
            item_y > TOUCH_SCREEN_HEIGHT - TOOLBAR_HEIGHT) {
            continue;
        }

        // Background
        uint32_t bg = (i == fm.selected_index || file->selected) ?
                     TOUCH_COLOR_PRIMARY : TOUCH_COLOR_SURFACE;
        touch_draw_rounded_rect(x, item_y, TOUCH_SCREEN_WIDTH - SIDEBAR_WIDTH - 40,
                               item_height - 10, 10, bg);

        // Icon
        touch_draw_emoji(x + 15, item_y + 10, get_file_icon(file->type), 60);

        // Filename
        touch_draw_text(x + 90, item_y + 15, file->name, TOUCH_COLOR_TEXT, 22);

        // File info
        char info[128];
        if (file->is_directory) {
            snprintf(info, sizeof(info), "Folder");
        } else {
            float size_kb = file->size / 1024.0f;
            if (size_kb < 1024) {
                snprintf(info, sizeof(info), "%.1f KB", size_kb);
            } else {
                snprintf(info, sizeof(info), "%.1f MB", size_kb / 1024.0f);
            }
        }
        touch_draw_text(x + 90, item_y + 50, info, TOUCH_COLOR_TEXT_DIM, 18);

        // Date
        char date_str[64];
        struct tm* tm_info = localtime(&file->modified);
        strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M", tm_info);
        touch_draw_text(TOUCH_SCREEN_WIDTH - 250, item_y + 30,
                       date_str, TOUCH_COLOR_TEXT_DIM, 18);
    }
}

void draw_toolbar(void) {
    int y = TOUCH_SCREEN_HEIGHT - TOOLBAR_HEIGHT;
    touch_draw_rect(0, y, TOUCH_SCREEN_WIDTH, TOOLBAR_HEIGHT, TOUCH_COLOR_SURFACE);

    int x = SIDEBAR_WIDTH + 40;
    int btn_width = 180;
    int spacing = 20;

    // Action buttons
    touch_button_t* new_folder = touch_button_create(x, y + 15, btn_width, 70, "📁 New");
    touch_button_draw(new_folder);
    x += btn_width + spacing;

    touch_button_t* copy_btn = touch_button_create(x, y + 15, btn_width, 70, "📋 Copy");
    touch_button_draw(copy_btn);
    x += btn_width + spacing;

    touch_button_t* move_btn = touch_button_create(x, y + 15, btn_width, 70, "✂️ Cut");
    touch_button_draw(move_btn);
    x += btn_width + spacing;

    touch_button_t* paste_btn = touch_button_create(x, y + 15, btn_width, 70, "📌 Paste");
    touch_button_draw(paste_btn);
    x += btn_width + spacing;

    touch_button_t* delete_btn = touch_button_create(x, y + 15, btn_width, 70, "🗑️ Delete");
    touch_button_draw(delete_btn);

    // Multi-select toggle
    const char* select_text = fm.multi_select_mode ? "✓ Select" : "Select";
    touch_draw_text(TOUCH_SCREEN_WIDTH - 200, y + 35, select_text, TOUCH_COLOR_TEXT, 20);
}

void render_file_manager(void) {
    // Background
    touch_clear_screen(TOUCH_COLOR_BG);

    // Draw components
    draw_breadcrumb_nav();
    draw_sidebar();

    if (fm.grid_view) {
        draw_file_grid();
    } else {
        draw_file_list();
    }

    draw_toolbar();

    // Context menu if visible
    if (fm.show_context_menu) {
        touch_draw_rounded_rect(fm.context_menu_x, fm.context_menu_y,
                               280, 300, 10, TOUCH_COLOR_SURFACE);

        int y = fm.context_menu_y + 20;
        touch_draw_text(fm.context_menu_x + 20, y, "📂 Open", TOUCH_COLOR_TEXT, 22);
        y += 50;
        touch_draw_text(fm.context_menu_x + 20, y, "✏️ Rename", TOUCH_COLOR_TEXT, 22);
        y += 50;
        touch_draw_text(fm.context_menu_x + 20, y, "📋 Copy", TOUCH_COLOR_TEXT, 22);
        y += 50;
        touch_draw_text(fm.context_menu_x + 20, y, "✂️ Cut", TOUCH_COLOR_TEXT, 22);
        y += 50;
        touch_draw_text(fm.context_menu_x + 20, y, "🗑️ Delete", TOUCH_COLOR_TEXT, 22);
        y += 50;
        touch_draw_text(fm.context_menu_x + 20, y, "ℹ️ Properties", TOUCH_COLOR_TEXT, 22);
    }

    touch_flip_buffer();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("TouchOS File Manager\n\n");

    // Initialize
    touch_framework_init();

    // Set initial state
    fm.grid_view = true;
    fm.show_hidden = false;
    fm.multi_select_mode = false;
    fm.show_context_menu = false;

    // Load home directory
    const char* home = getenv("HOME");
    if (!home) home = "/home";

    load_directory(home);

    // Main loop
    while (1) {
        render_file_manager();
        touch_sleep_ms(16);  // 60 FPS
    }

    touch_framework_shutdown();
    return 0;
}
