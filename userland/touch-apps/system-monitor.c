// userland/touch-apps/system-monitor.c
// TouchOS System Monitor - Touch-Optimized Task Manager
// Created by: floof<3

#include "../libtouch/touch_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/sysinfo.h>
#include <sys/statvfs.h>
#include <time.h>

// System monitor constants
#define MAX_PROCESSES 256
#define GRAPH_WIDTH 800
#define GRAPH_HEIGHT 200
#define HISTORY_SIZE 60  // 60 seconds of history

// Process info
typedef struct {
    int pid;
    char name[256];
    char state;
    unsigned long cpu_time;
    unsigned long memory_kb;
    float cpu_percent;
    int uid;
} process_info_t;

// System stats
typedef struct {
    // CPU
    float cpu_usage;
    float cpu_history[HISTORY_SIZE];
    int history_pos;

    // Memory
    unsigned long total_memory_kb;
    unsigned long free_memory_kb;
    unsigned long used_memory_kb;
    unsigned long cached_memory_kb;
    float memory_percent;
    float memory_history[HISTORY_SIZE];

    // Processes
    process_info_t processes[MAX_PROCESSES];
    int process_count;
    int selected_process;

    // Disk
    unsigned long disk_total_kb;
    unsigned long disk_free_kb;
    unsigned long disk_used_kb;
    float disk_percent;

    // Network
    unsigned long net_rx_bytes;
    unsigned long net_tx_bytes;
    float net_rx_kb_sec;
    float net_tx_kb_sec;

    // UI state
    int selected_tab;  // 0=Overview, 1=Processes, 2=Performance, 3=Disk
    int scroll_offset;
    bool show_kill_dialog;

    // Timing
    uint64_t last_update;
    unsigned long last_cpu_total;
    unsigned long last_cpu_idle;

} system_monitor_t;

static system_monitor_t monitor = {0};

// ============================================================================
// System Information Gathering
// ============================================================================

void read_cpu_stats(void) {
    FILE* f = fopen("/proc/stat", "r");
    if (!f) return;

    unsigned long user, nice, system, idle, iowait, irq, softirq;
    fscanf(f, "cpu %lu %lu %lu %lu %lu %lu %lu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(f);

    unsigned long total = user + nice + system + idle + iowait + irq + softirq;
    unsigned long total_diff = total - monitor.last_cpu_total;
    unsigned long idle_diff = idle - monitor.last_cpu_idle;

    if (total_diff > 0) {
        monitor.cpu_usage = 100.0f * (1.0f - ((float)idle_diff / (float)total_diff));
    }

    monitor.last_cpu_total = total;
    monitor.last_cpu_idle = idle;

    // Add to history
    monitor.cpu_history[monitor.history_pos] = monitor.cpu_usage;
    monitor.history_pos = (monitor.history_pos + 1) % HISTORY_SIZE;
}

void read_memory_stats(void) {
    struct sysinfo si;
    if (sysinfo(&si) == 0) {
        monitor.total_memory_kb = si.totalram / 1024;
        monitor.free_memory_kb = si.freeram / 1024;
        monitor.cached_memory_kb = si.bufferram / 1024;
        monitor.used_memory_kb = monitor.total_memory_kb - monitor.free_memory_kb;
        monitor.memory_percent = 100.0f * (float)monitor.used_memory_kb / (float)monitor.total_memory_kb;

        // Add to history
        monitor.memory_history[monitor.history_pos % HISTORY_SIZE] = monitor.memory_percent;
    }
}

void read_disk_stats(void) {
    struct statvfs vfs;
    if (statvfs("/", &vfs) == 0) {
        monitor.disk_total_kb = (vfs.f_blocks * vfs.f_frsize) / 1024;
        monitor.disk_free_kb = (vfs.f_bfree * vfs.f_frsize) / 1024;
        monitor.disk_used_kb = monitor.disk_total_kb - monitor.disk_free_kb;
        monitor.disk_percent = 100.0f * (float)monitor.disk_used_kb / (float)monitor.disk_total_kb;
    }
}

void read_process_list(void) {
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) return;

    monitor.process_count = 0;
    struct dirent* entry;

    while ((entry = readdir(proc_dir)) != NULL && monitor.process_count < MAX_PROCESSES) {
        // Check if directory name is a number (PID)
        if (entry->d_name[0] < '0' || entry->d_name[0] > '9') continue;

        int pid = atoi(entry->d_name);
        process_info_t* proc = &monitor.processes[monitor.process_count];

        proc->pid = pid;

        // Read /proc/[pid]/stat
        char stat_path[256];
        snprintf(stat_path, sizeof(stat_path), "/proc/%d/stat", pid);

        FILE* f = fopen(stat_path, "r");
        if (!f) continue;

        char state;
        unsigned long utime, stime;
        long rss;

        fscanf(f, "%*d (%255[^)]) %c %*d %*d %*d %*d %*d %*u %*u %*u %*u %*u %lu %lu %*d %*d %*d %*d %*d %*d %*u %*u %ld",
               proc->name, &state, &utime, &stime, &rss);

        fclose(f);

        proc->state = state;
        proc->cpu_time = utime + stime;
        proc->memory_kb = (rss * 4);  // Pages to KB (assuming 4KB pages)
        proc->cpu_percent = 0.0f;  // Would need delta calculation

        monitor.process_count++;
    }

    closedir(proc_dir);

    // Sort by memory usage (descending)
    for (int i = 0; i < monitor.process_count - 1; i++) {
        for (int j = i + 1; j < monitor.process_count; j++) {
            if (monitor.processes[i].memory_kb < monitor.processes[j].memory_kb) {
                process_info_t temp = monitor.processes[i];
                monitor.processes[i] = monitor.processes[j];
                monitor.processes[j] = temp;
            }
        }
    }
}

void update_stats(void) {
    uint64_t now = touch_get_time_ms();

    // Update every second
    if (now - monitor.last_update < 1000) return;

    monitor.last_update = now;

    read_cpu_stats();
    read_memory_stats();
    read_disk_stats();
    read_process_list();
}

// ============================================================================
// UI Components
// ============================================================================

void draw_tabs(void) {
    int tab_width = TOUCH_SCREEN_WIDTH / 4;
    int tab_height = 80;

    const char* tabs[] = {"📊 Overview", "⚙️ Processes", "📈 Performance", "💾 Disk"};

    for (int i = 0; i < 4; i++) {
        int x = i * tab_width;
        uint32_t bg = (i == monitor.selected_tab) ? TOUCH_COLOR_PRIMARY : TOUCH_COLOR_SURFACE;

        touch_draw_rect(x, 0, tab_width, tab_height, bg);
        touch_draw_text_centered(
            &(touch_rect_t){x, 0, tab_width, tab_height},
            tabs[i], TOUCH_COLOR_TEXT, 22);
    }
}

void draw_stat_card(int x, int y, int w, int h, const char* title,
                   const char* value, const char* subtitle, float percent) {
    touch_draw_rounded_rect(x, y, w, h, 10, TOUCH_COLOR_SURFACE);

    // Title
    touch_draw_text(x + 20, y + 20, title, TOUCH_COLOR_TEXT_DIM, 20);

    // Value
    touch_draw_text(x + 20, y + 60, value, TOUCH_COLOR_TEXT, 36);

    // Subtitle
    if (subtitle) {
        touch_draw_text(x + 20, y + 110, subtitle, TOUCH_COLOR_TEXT_DIM, 18);
    }

    // Progress bar
    if (percent >= 0) {
        int bar_width = w - 40;
        int bar_height = 20;
        int bar_y = y + h - 40;

        // Background
        touch_draw_rounded_rect(x + 20, bar_y, bar_width, bar_height, 5, TOUCH_COLOR_SECONDARY);

        // Fill
        int fill_width = (int)(bar_width * (percent / 100.0f));
        uint32_t fill_color = percent > 90 ? TOUCH_COLOR_ERROR :
                             percent > 70 ? TOUCH_COLOR_WARNING :
                             TOUCH_COLOR_SUCCESS;
        touch_draw_rounded_rect(x + 20, bar_y, fill_width, bar_height, 5, fill_color);

        // Percentage text
        char pct_text[16];
        snprintf(pct_text, sizeof(pct_text), "%.1f%%", percent);
        touch_draw_text_centered(
            &(touch_rect_t){x + 20, bar_y - 2, bar_width, bar_height},
            pct_text, TOUCH_COLOR_TEXT, 16);
    }
}

void draw_overview_tab(void) {
    int y = 120;
    int card_width = 440;
    int card_height = 180;
    int margin = 20;

    // CPU card
    char cpu_text[32];
    snprintf(cpu_text, sizeof(cpu_text), "%.1f%%", monitor.cpu_usage);
    draw_stat_card(margin, y, card_width, card_height,
                  "CPU Usage", cpu_text, "Intel i5-8250U @ 1.6GHz",
                  monitor.cpu_usage);

    // Memory card
    char mem_text[32];
    snprintf(mem_text, sizeof(mem_text), "%.1f GB", monitor.used_memory_kb / 1024.0f / 1024.0f);
    char mem_subtitle[64];
    snprintf(mem_subtitle, sizeof(mem_subtitle), "of %.1f GB",
            monitor.total_memory_kb / 1024.0f / 1024.0f);
    draw_stat_card(margin + card_width + margin, y, card_width, card_height,
                  "Memory", mem_text, mem_subtitle, monitor.memory_percent);

    // Disk card
    y += card_height + margin;
    char disk_text[32];
    snprintf(disk_text, sizeof(disk_text), "%.1f GB", monitor.disk_used_kb / 1024.0f / 1024.0f);
    char disk_subtitle[64];
    snprintf(disk_subtitle, sizeof(disk_subtitle), "of %.1f GB",
            monitor.disk_total_kb / 1024.0f / 1024.0f);
    draw_stat_card(margin, y, card_width, card_height,
                  "Disk Usage", disk_text, disk_subtitle, monitor.disk_percent);

    // Processes card
    char proc_text[32];
    snprintf(proc_text, sizeof(proc_text), "%d", monitor.process_count);
    draw_stat_card(margin + card_width + margin, y, card_width, card_height,
                  "Processes", proc_text, "Running processes", -1);

    // System info
    y += card_height + margin;
    touch_draw_text(margin, y, "💻 System Information", TOUCH_COLOR_TEXT, 24);
    y += 50;

    char info_lines[6][128];
    snprintf(info_lines[0], 128, "OS: TouchOS v1.0");
    snprintf(info_lines[1], 128, "Kernel: Linux 6.12.48");
    snprintf(info_lines[2], 128, "CPU: Intel Core i5-8250U @ 1.6GHz");
    snprintf(info_lines[3], 128, "RAM: 8GB LPDDR3");
    snprintf(info_lines[4], 128, "Graphics: Intel UHD 620");
    snprintf(info_lines[5], 128, "Display: Acer T230H (1920x1080)");

    for (int i = 0; i < 6; i++) {
        touch_draw_text(margin + 20, y, info_lines[i], TOUCH_COLOR_TEXT_DIM, 20);
        y += 35;
    }
}

void draw_process_list_tab(void) {
    int y = 120;
    int item_height = 80;

    // Header
    touch_draw_rect(0, y, TOUCH_SCREEN_WIDTH, 60, TOUCH_COLOR_SURFACE);
    touch_draw_text(40, y + 20, "Process", TOUCH_COLOR_TEXT, 20);
    touch_draw_text(500, y + 20, "PID", TOUCH_COLOR_TEXT, 20);
    touch_draw_text(700, y + 20, "Memory", TOUCH_COLOR_TEXT, 20);
    touch_draw_text(1000, y + 20, "State", TOUCH_COLOR_TEXT, 20);

    y += 70;

    // Process list
    for (int i = 0; i < monitor.process_count; i++) {
        int item_y = y + i * item_height - monitor.scroll_offset;

        if (item_y + item_height < 180 || item_y > TOUCH_SCREEN_HEIGHT - 100) {
            continue;
        }

        process_info_t* proc = &monitor.processes[i];

        // Background
        uint32_t bg = (i == monitor.selected_process) ? TOUCH_COLOR_PRIMARY : TOUCH_COLOR_BG;
        if (i % 2 == 0 && i != monitor.selected_process) {
            bg = TOUCH_COLOR_SURFACE;
        }
        touch_draw_rect(0, item_y, TOUCH_SCREEN_WIDTH, item_height - 5, bg);

        // Process name
        char name_truncated[40];
        strncpy(name_truncated, proc->name, sizeof(name_truncated) - 1);
        if (strlen(proc->name) > 35) {
            name_truncated[32] = '.';
            name_truncated[33] = '.';
            name_truncated[34] = '.';
            name_truncated[35] = '\0';
        }
        touch_draw_text(40, item_y + 25, name_truncated, TOUCH_COLOR_TEXT, 20);

        // PID
        char pid_str[16];
        snprintf(pid_str, sizeof(pid_str), "%d", proc->pid);
        touch_draw_text(500, item_y + 25, pid_str, TOUCH_COLOR_TEXT_DIM, 20);

        // Memory
        char mem_str[32];
        if (proc->memory_kb > 1024) {
            snprintf(mem_str, sizeof(mem_str), "%.1f MB", proc->memory_kb / 1024.0f);
        } else {
            snprintf(mem_str, sizeof(mem_str), "%lu KB", proc->memory_kb);
        }
        touch_draw_text(700, item_y + 25, mem_str, TOUCH_COLOR_TEXT_DIM, 20);

        // State
        const char* state_str;
        switch (proc->state) {
            case 'R': state_str = "Running"; break;
            case 'S': state_str = "Sleeping"; break;
            case 'D': state_str = "Waiting"; break;
            case 'Z': state_str = "Zombie"; break;
            case 'T': state_str = "Stopped"; break;
            default: state_str = "Unknown"; break;
        }
        touch_draw_text(1000, item_y + 25, state_str, TOUCH_COLOR_TEXT_DIM, 20);
    }

    // Kill button (if process selected)
    if (monitor.selected_process >= 0) {
        touch_button_t* kill_btn = touch_button_create(
            TOUCH_SCREEN_WIDTH - 220, TOUCH_SCREEN_HEIGHT - 180,
            200, 70, "🗑️ Kill Process");
        touch_button_draw(kill_btn);
    }
}

void draw_performance_tab(void) {
    int y = 120;

    // CPU graph
    touch_draw_text(40, y, "💻 CPU Usage History", TOUCH_COLOR_TEXT, 24);
    y += 50;

    int graph_x = 40;
    int graph_y = y;

    touch_draw_rounded_rect(graph_x, graph_y, GRAPH_WIDTH, GRAPH_HEIGHT, 10, TOUCH_COLOR_SURFACE);

    // Draw grid lines
    for (int i = 0; i <= 4; i++) {
        int grid_y = graph_y + (GRAPH_HEIGHT * i / 4);
        touch_draw_line(graph_x, grid_y, graph_x + GRAPH_WIDTH, grid_y,
                       TOUCH_COLOR_SECONDARY, 1);

        char pct[8];
        snprintf(pct, sizeof(pct), "%d%%", 100 - (i * 25));
        touch_draw_text(graph_x + GRAPH_WIDTH + 10, grid_y - 10, pct, TOUCH_COLOR_TEXT_DIM, 16);
    }

    // Draw CPU history line
    for (int i = 1; i < HISTORY_SIZE; i++) {
        int prev_idx = (monitor.history_pos + i - 1) % HISTORY_SIZE;
        int curr_idx = (monitor.history_pos + i) % HISTORY_SIZE;

        float prev_val = monitor.cpu_history[prev_idx];
        float curr_val = monitor.cpu_history[curr_idx];

        int x1 = graph_x + (i - 1) * (GRAPH_WIDTH / HISTORY_SIZE);
        int y1 = graph_y + GRAPH_HEIGHT - (int)(GRAPH_HEIGHT * prev_val / 100.0f);
        int x2 = graph_x + i * (GRAPH_WIDTH / HISTORY_SIZE);
        int y2 = graph_y + GRAPH_HEIGHT - (int)(GRAPH_HEIGHT * curr_val / 100.0f);

        touch_draw_line(x1, y1, x2, y2, TOUCH_COLOR_PRIMARY, 3);
    }

    y += GRAPH_HEIGHT + 40;

    // Memory graph
    touch_draw_text(40, y, "💾 Memory Usage History", TOUCH_COLOR_TEXT, 24);
    y += 50;

    graph_y = y;
    touch_draw_rounded_rect(graph_x, graph_y, GRAPH_WIDTH, GRAPH_HEIGHT, 10, TOUCH_COLOR_SURFACE);

    // Draw memory history line
    for (int i = 1; i < HISTORY_SIZE; i++) {
        int prev_idx = (monitor.history_pos + i - 1) % HISTORY_SIZE;
        int curr_idx = (monitor.history_pos + i) % HISTORY_SIZE;

        float prev_val = monitor.memory_history[prev_idx];
        float curr_val = monitor.memory_history[curr_idx];

        int x1 = graph_x + (i - 1) * (GRAPH_WIDTH / HISTORY_SIZE);
        int y1 = graph_y + GRAPH_HEIGHT - (int)(GRAPH_HEIGHT * prev_val / 100.0f);
        int x2 = graph_x + i * (GRAPH_WIDTH / HISTORY_SIZE);
        int y2 = graph_y + GRAPH_HEIGHT - (int)(GRAPH_HEIGHT * curr_val / 100.0f);

        touch_draw_line(x1, y1, x2, y2, TOUCH_COLOR_SUCCESS, 3);
    }
}

void draw_disk_tab(void) {
    int y = 120;

    touch_draw_text(40, y, "💾 Disk Usage", TOUCH_COLOR_TEXT, 28);
    y += 60;

    // Disk pie chart (simplified as bar)
    int bar_width = 1200;
    int bar_height = 60;

    touch_draw_rounded_rect(40, y, bar_width, bar_height, 10, TOUCH_COLOR_SECONDARY);

    int used_width = (int)(bar_width * (monitor.disk_percent / 100.0f));
    touch_draw_rounded_rect(40, y, used_width, bar_height, 10, TOUCH_COLOR_PRIMARY);

    char disk_info[128];
    snprintf(disk_info, sizeof(disk_info), "%.1f GB used of %.1f GB (%.1f%%)",
            monitor.disk_used_kb / 1024.0f / 1024.0f,
            monitor.disk_total_kb / 1024.0f / 1024.0f,
            monitor.disk_percent);
    touch_draw_text_centered(&(touch_rect_t){40, y, bar_width, bar_height},
                            disk_info, TOUCH_COLOR_TEXT, 22);

    y += 100;

    // Disk details
    touch_draw_text(40, y, "📁 Filesystem: /", TOUCH_COLOR_TEXT_DIM, 20);
    y += 40;
    touch_draw_text(40, y, "💿 Type: ext4", TOUCH_COLOR_TEXT_DIM, 20);
    y += 40;
    char free_text[64];
    snprintf(free_text, sizeof(free_text), "🆓 Free: %.1f GB",
            monitor.disk_free_kb / 1024.0f / 1024.0f);
    touch_draw_text(40, y, free_text, TOUCH_COLOR_TEXT_DIM, 20);
}

void render_system_monitor(void) {
    // Background
    touch_clear_screen(TOUCH_COLOR_BG);

    // Tabs
    draw_tabs();

    // Tab content
    switch (monitor.selected_tab) {
        case 0: draw_overview_tab(); break;
        case 1: draw_process_list_tab(); break;
        case 2: draw_performance_tab(); break;
        case 3: draw_disk_tab(); break;
    }

    touch_flip_buffer();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;

    printf("TouchOS System Monitor\n\n");

    // Initialize
    touch_framework_init();

    monitor.selected_tab = 0;
    monitor.selected_process = -1;
    monitor.scroll_offset = 0;
    monitor.last_update = 0;

    // Initial stats
    update_stats();

    // Main loop
    while (1) {
        update_stats();
        render_system_monitor();
        touch_sleep_ms(100);  // 10 FPS (stats update every 1s)
    }

    touch_framework_shutdown();
    return 0;
}
