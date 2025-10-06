// userland/touch-apps/terminal.c
// TouchOS Terminal - Touch-Optimized Terminal Emulator
// Created by: floof<3

#include "../libtouch/touch_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pty.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <termios.h>

// Terminal constants
#define TERM_COLS 80
#define TERM_ROWS 24
#define CHAR_WIDTH 18
#define CHAR_HEIGHT 32
#define SCROLLBACK_SIZE 1000

// Terminal state
typedef struct {
    char screen[TERM_ROWS][TERM_COLS + 1];
    char scrollback[SCROLLBACK_SIZE][TERM_COLS + 1];
    int scrollback_pos;
    int scrollback_count;

    int cursor_x;
    int cursor_y;
    int scroll_offset;

    char input_buffer[256];
    int input_pos;

    bool keyboard_visible;
    bool ctrl_pressed;
    bool alt_pressed;

    // PTY
    int pty_master;
    pid_t shell_pid;

    // Visual
    uint32_t bg_color;
    uint32_t fg_color;
    uint32_t cursor_color;

    // Touch
    int last_tap_time;
    int last_tap_x;
    int last_tap_y;

} terminal_t;

static terminal_t term = {0};

// ============================================================================
// Terminal Backend
// ============================================================================

void term_write_char(char c) {
    if (c == '\n') {
        // Newline
        term.cursor_x = 0;
        term.cursor_y++;

        if (term.cursor_y >= TERM_ROWS) {
            // Scroll up - save to scrollback
            if (term.scrollback_count < SCROLLBACK_SIZE) {
                memcpy(term.scrollback[term.scrollback_count], term.screen[0], TERM_COLS + 1);
                term.scrollback_count++;
            } else {
                // Circular buffer
                memcpy(term.scrollback[term.scrollback_pos], term.screen[0], TERM_COLS + 1);
                term.scrollback_pos = (term.scrollback_pos + 1) % SCROLLBACK_SIZE;
            }

            // Move all lines up
            for (int i = 0; i < TERM_ROWS - 1; i++) {
                memcpy(term.screen[i], term.screen[i + 1], TERM_COLS + 1);
            }
            memset(term.screen[TERM_ROWS - 1], 0, TERM_COLS + 1);
            term.cursor_y = TERM_ROWS - 1;
        }
    } else if (c == '\r') {
        term.cursor_x = 0;
    } else if (c == '\b') {
        // Backspace
        if (term.cursor_x > 0) {
            term.cursor_x--;
            term.screen[term.cursor_y][term.cursor_x] = ' ';
        }
    } else if (c == '\t') {
        // Tab (4 spaces)
        for (int i = 0; i < 4 && term.cursor_x < TERM_COLS; i++) {
            term.screen[term.cursor_y][term.cursor_x] = ' ';
            term.cursor_x++;
        }
    } else if (c >= 32 && c < 127) {
        // Printable character
        if (term.cursor_x < TERM_COLS) {
            term.screen[term.cursor_y][term.cursor_x] = c;
            term.cursor_x++;
        }

        if (term.cursor_x >= TERM_COLS) {
            term.cursor_x = 0;
            term.cursor_y++;

            if (term.cursor_y >= TERM_ROWS) {
                term.cursor_y = TERM_ROWS - 1;
                // Scroll (same as newline)
            }
        }
    }
}

void term_write_string(const char* str) {
    while (*str) {
        term_write_char(*str);
        str++;
    }
}

void term_clear(void) {
    memset(term.screen, 0, sizeof(term.screen));
    term.cursor_x = 0;
    term.cursor_y = 0;
}

void term_send_to_shell(const char* str) {
    if (term.pty_master >= 0) {
        write(term.pty_master, str, strlen(str));
    }
}

void term_read_from_shell(void) {
    char buffer[256];
    int flags = fcntl(term.pty_master, F_GETFL, 0);
    fcntl(term.pty_master, F_SETFL, flags | O_NONBLOCK);

    ssize_t n = read(term.pty_master, buffer, sizeof(buffer) - 1);
    if (n > 0) {
        buffer[n] = '\0';
        for (ssize_t i = 0; i < n; i++) {
            term_write_char(buffer[i]);
        }
    }
}

void term_spawn_shell(void) {
    struct winsize ws = {
        .ws_row = TERM_ROWS,
        .ws_col = TERM_COLS,
        .ws_xpixel = 0,
        .ws_ypixel = 0
    };

    term.shell_pid = forkpty(&term.pty_master, NULL, NULL, &ws);

    if (term.shell_pid == 0) {
        // Child process - execute shell
        setenv("TERM", "xterm-256color", 1);
        execlp("/bin/sh", "sh", NULL);
        exit(1);
    } else if (term.shell_pid < 0) {
        printf("Failed to fork shell\n");
        term.pty_master = -1;
    }
}

// ============================================================================
// On-Screen Keyboard
// ============================================================================

void draw_keyboard(void) {
    int kb_height = TOUCH_OSK_HEIGHT;
    int y = TOUCH_SCREEN_HEIGHT - kb_height;

    // Keyboard background
    touch_draw_rect(0, y, TOUCH_SCREEN_WIDTH, kb_height, TOUCH_COLOR_SURFACE);

    int key_width = TOUCH_KEY_WIDTH;
    int key_height = TOUCH_KEY_HEIGHT;
    int margin = 8;

    // Normal QWERTY layout
    const char* rows[] = {
        "1234567890-=",
        "qwertyuiop[]",
        "asdfghjkl;'",
        "zxcvbnm,./"
    };

    int start_y = y + 10;

    for (int row = 0; row < 4; row++) {
        int row_len = strlen(rows[row]);
        int total_width = row_len * (key_width + margin);
        int start_x = (TOUCH_SCREEN_WIDTH - total_width) / 2;

        for (int col = 0; col < row_len; col++) {
            int kx = start_x + col * (key_width + margin);
            int ky = start_y + row * (key_height + margin);

            uint32_t key_color = TOUCH_COLOR_SECONDARY;
            touch_draw_rounded_rect(kx, ky, key_width, key_height, 8, key_color);

            char key_str[2] = {rows[row][col], '\0'};
            touch_draw_text_centered(
                &(touch_rect_t){kx, ky, key_width, key_height},
                key_str, TOUCH_COLOR_TEXT, 28);
        }
    }

    // Bottom row - special keys
    int bottom_y = start_y + 4 * (key_height + margin);

    // Ctrl
    int x = 20;
    uint32_t ctrl_color = term.ctrl_pressed ? TOUCH_COLOR_PRIMARY : TOUCH_COLOR_SECONDARY;
    touch_draw_rounded_rect(x, bottom_y, 150, key_height, 8, ctrl_color);
    touch_draw_text_centered(&(touch_rect_t){x, bottom_y, 150, key_height},
                            "Ctrl", TOUCH_COLOR_TEXT, 24);

    // Alt
    x += 160;
    uint32_t alt_color = term.alt_pressed ? TOUCH_COLOR_PRIMARY : TOUCH_COLOR_SECONDARY;
    touch_draw_rounded_rect(x, bottom_y, 150, key_height, 8, alt_color);
    touch_draw_text_centered(&(touch_rect_t){x, bottom_y, 150, key_height},
                            "Alt", TOUCH_COLOR_TEXT, 24);

    // Space bar (large)
    x += 160;
    touch_draw_rounded_rect(x, bottom_y, 800, key_height, 8, TOUCH_COLOR_SECONDARY);
    touch_draw_text_centered(&(touch_rect_t){x, bottom_y, 800, key_height},
                            "Space", TOUCH_COLOR_TEXT, 24);

    // Backspace
    x += 810;
    touch_draw_rounded_rect(x, bottom_y, 200, key_height, 8, TOUCH_COLOR_SECONDARY);
    touch_draw_text_centered(&(touch_rect_t){x, bottom_y, 200, key_height},
                            "⌫ Bksp", TOUCH_COLOR_TEXT, 24);

    // Enter
    x += 210;
    touch_draw_rounded_rect(x, bottom_y, 200, key_height, 8, TOUCH_COLOR_PRIMARY);
    touch_draw_text_centered(&(touch_rect_t){x, bottom_y, 200, key_height},
                            "↵ Enter", TOUCH_COLOR_TEXT, 24);

    // Tab
    x += 210;
    touch_draw_rounded_rect(x, bottom_y, 150, key_height, 8, TOUCH_COLOR_SECONDARY);
    touch_draw_text_centered(&(touch_rect_t){x, bottom_y, 150, key_height},
                            "Tab", TOUCH_COLOR_TEXT, 24);

    // Esc
    x += 160;
    touch_draw_rounded_rect(x, bottom_y, 150, key_height, 8, TOUCH_COLOR_SECONDARY);
    touch_draw_text_centered(&(touch_rect_t){x, bottom_y, 150, key_height},
                            "Esc", TOUCH_COLOR_TEXT, 24);
}

void handle_keyboard_tap(int x, int y) {
    int kb_height = TOUCH_OSK_HEIGHT;
    int kb_y = TOUCH_SCREEN_HEIGHT - kb_height;

    if (y < kb_y) return;

    // Normalize to keyboard coordinates
    y -= kb_y;

    int key_width = TOUCH_KEY_WIDTH;
    int key_height = TOUCH_KEY_HEIGHT;
    int margin = 8;

    const char* rows[] = {
        "1234567890-=",
        "qwertyuiop[]",
        "asdfghjkl;'",
        "zxcvbnm,./"
    };

    int start_y = 10;

    // Check main keys
    for (int row = 0; row < 4; row++) {
        int row_len = strlen(rows[row]);
        int total_width = row_len * (key_width + margin);
        int start_x = (TOUCH_SCREEN_WIDTH - total_width) / 2;

        int ky = start_y + row * (key_height + margin);

        if (y >= ky && y <= ky + key_height) {
            for (int col = 0; col < row_len; col++) {
                int kx = start_x + col * (key_width + margin);

                if (x >= kx && x <= kx + key_width) {
                    char key_char = rows[row][col];

                    // Send to shell
                    char buf[2] = {key_char, '\0'};
                    term_send_to_shell(buf);
                    return;
                }
            }
        }
    }

    // Check special keys
    int bottom_y = start_y + 4 * (key_height + margin);

    if (y >= bottom_y && y <= bottom_y + key_height) {
        int kx = 20;

        // Ctrl
        if (x >= kx && x <= kx + 150) {
            term.ctrl_pressed = !term.ctrl_pressed;
            return;
        }
        kx += 160;

        // Alt
        if (x >= kx && x <= kx + 150) {
            term.alt_pressed = !term.alt_pressed;
            return;
        }
        kx += 160;

        // Space
        if (x >= kx && x <= kx + 800) {
            term_send_to_shell(" ");
            return;
        }
        kx += 810;

        // Backspace
        if (x >= kx && x <= kx + 200) {
            term_send_to_shell("\b");
            return;
        }
        kx += 210;

        // Enter
        if (x >= kx && x <= kx + 200) {
            term_send_to_shell("\n");
            return;
        }
        kx += 210;

        // Tab
        if (x >= kx && x <= kx + 150) {
            term_send_to_shell("\t");
            return;
        }
        kx += 160;

        // Esc
        if (x >= kx && x <= kx + 150) {
            term_send_to_shell("\x1b");
            return;
        }
    }
}

// ============================================================================
// Terminal Display
// ============================================================================

void draw_terminal_screen(void) {
    // Terminal area
    int term_height = term.keyboard_visible ?
                     TOUCH_SCREEN_HEIGHT - TOUCH_OSK_HEIGHT - 100 :
                     TOUCH_SCREEN_HEIGHT - 100;

    touch_draw_rect(0, 100, TOUCH_SCREEN_WIDTH, term_height, term.bg_color);

    // Draw terminal text
    int start_x = 20;
    int start_y = 120;

    for (int row = 0; row < TERM_ROWS; row++) {
        int y = start_y + row * CHAR_HEIGHT;

        if (y + CHAR_HEIGHT > 100 + term_height) break;

        // Draw line
        touch_draw_text(start_x, y, term.screen[row], term.fg_color, 24);
    }

    // Draw cursor (blinking)
    uint64_t time_ms = touch_get_time_ms();
    if ((time_ms / 500) % 2 == 0) {
        int cursor_x = start_x + term.cursor_x * CHAR_WIDTH;
        int cursor_y = start_y + term.cursor_y * CHAR_HEIGHT;
        touch_draw_rect(cursor_x, cursor_y, CHAR_WIDTH, CHAR_HEIGHT, term.cursor_color);
    }
}

void draw_toolbar(void) {
    // Top toolbar
    touch_draw_rect(0, 0, TOUCH_SCREEN_WIDTH, 100, TOUCH_COLOR_SURFACE);

    // Title
    touch_draw_text(20, 30, "💻 Terminal", TOUCH_COLOR_TEXT, 32);

    // Keyboard toggle
    int x = TOUCH_SCREEN_WIDTH - 400;
    const char* kb_text = term.keyboard_visible ? "⌨️ Hide KB" : "⌨️ Show KB";
    touch_button_t* kb_btn = touch_button_create(x, 20, 180, 60, kb_text);
    touch_button_draw(kb_btn);

    // Clear button
    x += 190;
    touch_button_t* clear_btn = touch_button_create(x, 20, 180, 60, "🗑️ Clear");
    touch_button_draw(clear_btn);
}

void render_terminal(void) {
    // Background
    touch_clear_screen(TOUCH_COLOR_BG);

    // Draw components
    draw_toolbar();
    draw_terminal_screen();

    if (term.keyboard_visible) {
        draw_keyboard();
    }

    touch_flip_buffer();
}

// ============================================================================
// Touch Input Handling
// ============================================================================

void handle_terminal_tap(int x, int y) {
    // Toolbar buttons
    if (y >= 20 && y <= 80) {
        // Keyboard toggle
        if (x >= TOUCH_SCREEN_WIDTH - 400 && x <= TOUCH_SCREEN_WIDTH - 220) {
            term.keyboard_visible = !term.keyboard_visible;
            return;
        }

        // Clear button
        if (x >= TOUCH_SCREEN_WIDTH - 210 && x <= TOUCH_SCREEN_WIDTH - 30) {
            term_clear();
            term_send_to_shell("clear\n");
            return;
        }
    }

    // Keyboard input
    if (term.keyboard_visible && y >= TOUCH_SCREEN_HEIGHT - TOUCH_OSK_HEIGHT) {
        handle_keyboard_tap(x, y);
        return;
    }

    // Terminal area - show keyboard on tap
    if (!term.keyboard_visible && y >= 100) {
        term.keyboard_visible = true;
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("TouchOS Terminal\n\n");

    // Initialize
    touch_framework_init();

    // Set terminal state
    term.bg_color = 0xFF1A1A1A;
    term.fg_color = 0xFF00FF00;  // Green text (classic terminal)
    term.cursor_color = 0xFF00FF00;
    term.keyboard_visible = true;
    term.ctrl_pressed = false;
    term.alt_pressed = false;
    term.scroll_offset = 0;

    memset(term.screen, 0, sizeof(term.screen));
    term.cursor_x = 0;
    term.cursor_y = 0;

    // Spawn shell
    term_spawn_shell();

    // Welcome message
    term_write_string("TouchOS Terminal v1.0\n");
    term_write_string("Type 'help' for commands\n\n");

    // Main loop
    while (1) {
        // Read shell output
        term_read_from_shell();

        // Render
        render_terminal();

        // Handle touch events (simplified - would use framework events)
        touch_sleep_ms(16);  // 60 FPS
    }

    // Cleanup
    if (term.pty_master >= 0) {
        close(term.pty_master);
    }

    touch_framework_shutdown();
    return 0;
}
