// userland/touch-apps/installer.c
// TouchOS Graphical Installer - Ubuntu-style installer with touch interface
// Created by: floof<3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "../libtouch/touch_framework.h"

// ============================================================================
// Installer State Machine
// ============================================================================

typedef enum {
    INSTALLER_WELCOME,
    INSTALLER_DISK_SELECT,
    INSTALLER_PARTITION_CONFIRM,
    INSTALLER_INSTALLING,
    INSTALLER_COMPLETE,
    INSTALLER_ERROR
} installer_state_t;

typedef struct {
    char device[64];        // e.g., "/dev/sda"
    char size[32];          // e.g., "500GB"
    char model[128];        // e.g., "Samsung SSD 860 EVO"
    bool selected;
} disk_info_t;

typedef struct {
    installer_state_t state;
    disk_info_t disks[16];
    int disk_count;
    int selected_disk;

    // Installation progress
    int progress_percent;
    char progress_message[256];

    // UI elements
    touch_window_t* window;
    touch_button_t buttons[8];
    int button_count;
    touch_list_view_t* disk_list;

    bool installation_failed;
    char error_message[256];
} installer_context_t;

static installer_context_t ctx = {0};

// ============================================================================
// Disk Detection
// ============================================================================

void detect_disks() {
    ctx.disk_count = 0;

    // Try to read from installer_disks.txt (pre-populated)
    FILE* f = fopen("installer_disks.txt", "r");
    if (f) {
        char line[256];
        while (fgets(line, sizeof(line), f) && ctx.disk_count < 16) {
            disk_info_t* disk = &ctx.disks[ctx.disk_count];
            sscanf(line, "%[^,],%[^,],%[^\n]",
                   disk->device, disk->size, disk->model);
            disk->selected = false;
            ctx.disk_count++;
        }
        fclose(f);
    }

    // Fallback: scan /dev for block devices
    if (ctx.disk_count == 0) {
        // Simulate some disks for demo
        strcpy(ctx.disks[0].device, "/dev/sda");
        strcpy(ctx.disks[0].size, "500GB");
        strcpy(ctx.disks[0].model, "Primary Hard Drive");
        ctx.disks[0].selected = false;

        strcpy(ctx.disks[1].device, "/dev/sdb");
        strcpy(ctx.disks[1].size, "1TB");
        strcpy(ctx.disks[1].model, "Secondary Hard Drive");
        ctx.disks[1].selected = false;

        ctx.disk_count = 2;
    }
}

// ============================================================================
// Installation Functions
// ============================================================================

bool partition_disk(const char* device) {
    printf("Partitioning %s...\n", device);

    // Create partition table
    char cmd[512];

    // Clear existing partition table
    snprintf(cmd, sizeof(cmd),
             "dd if=/dev/zero of=%s bs=512 count=1 2>/dev/null", device);
    if (system(cmd) != 0) return false;

    // Create GPT partition table
    snprintf(cmd, sizeof(cmd),
             "parted -s %s mklabel gpt", device);
    if (system(cmd) != 0) return false;

    // Create EFI partition (512MB)
    snprintf(cmd, sizeof(cmd),
             "parted -s %s mkpart ESP fat32 1MiB 513MiB", device);
    if (system(cmd) != 0) return false;

    snprintf(cmd, sizeof(cmd),
             "parted -s %s set 1 esp on", device);
    if (system(cmd) != 0) return false;

    // Create root partition (rest of disk)
    snprintf(cmd, sizeof(cmd),
             "parted -s %s mkpart primary ext4 513MiB 100%%", device);
    if (system(cmd) != 0) return false;

    sync();
    sleep(1);

    return true;
}

bool format_partitions(const char* device) {
    printf("Formatting partitions...\n");

    char cmd[512];
    char efi_part[64], root_part[64];

    // Determine partition names
    snprintf(efi_part, sizeof(efi_part), "%s1", device);
    snprintf(root_part, sizeof(root_part), "%s2", device);

    // Format EFI partition as FAT32
    snprintf(cmd, sizeof(cmd),
             "mkfs.vfat -F32 %s", efi_part);
    if (system(cmd) != 0) return false;

    // Format root partition as ext4
    snprintf(cmd, sizeof(cmd),
             "mkfs.ext4 -F %s", root_part);
    if (system(cmd) != 0) return false;

    return true;
}

bool mount_partitions(const char* device) {
    printf("Mounting partitions...\n");

    char cmd[512];
    char root_part[64], efi_part[64];

    snprintf(efi_part, sizeof(efi_part), "%s1", device);
    snprintf(root_part, sizeof(root_part), "%s2", device);

    // Create mount points
    system("mkdir -p /mnt/touchos");
    system("mkdir -p /mnt/touchos/boot/efi");

    // Mount root partition
    snprintf(cmd, sizeof(cmd), "mount %s /mnt/touchos", root_part);
    if (system(cmd) != 0) return false;

    // Mount EFI partition
    snprintf(cmd, sizeof(cmd), "mount %s /mnt/touchos/boot/efi", efi_part);
    if (system(cmd) != 0) return false;

    return true;
}

bool install_system() {
    printf("Installing TouchOS system files...\n");

    // Create directory structure
    system("mkdir -p /mnt/touchos/boot");
    system("mkdir -p /mnt/touchos/bin");
    system("mkdir -p /mnt/touchos/lib");
    system("mkdir -p /mnt/touchos/etc");
    system("mkdir -p /mnt/touchos/home");
    system("mkdir -p /mnt/touchos/var");
    system("mkdir -p /mnt/touchos/tmp");

    // Copy kernel
    if (access("kernel.elf", F_OK) == 0) {
        if (system("cp kernel.elf /mnt/touchos/boot/") != 0) return false;
    } else if (access("/boot/kernel.elf", F_OK) == 0) {
        if (system("cp /boot/kernel.elf /mnt/touchos/boot/") != 0) return false;
    }

    // Copy userland applications
    if (access("userland", F_OK) == 0) {
        system("cp -r userland/* /mnt/touchos/bin/");
    }

    // Copy touch apps
    system("cp -r /bin/touch-* /mnt/touchos/bin/ 2>/dev/null");
    system("cp -r /lib/libtouch* /mnt/touchos/lib/ 2>/dev/null");

    return true;
}

bool install_bootloader(const char* device) {
    printf("Installing bootloader...\n");

    char cmd[512];

    // Install GRUB2 for UEFI
    snprintf(cmd, sizeof(cmd),
             "grub-install --target=x86_64-efi --efi-directory=/mnt/touchos/boot/efi "
             "--bootloader-id=TouchOS --boot-directory=/mnt/touchos/boot %s", device);
    if (system(cmd) != 0) {
        // Try legacy BIOS boot
        snprintf(cmd, sizeof(cmd),
                 "grub-install --target=i386-pc --boot-directory=/mnt/touchos/boot %s",
                 device);
        if (system(cmd) != 0) return false;
    }

    // Create GRUB configuration
    FILE* grub_cfg = fopen("/mnt/touchos/boot/grub/grub.cfg", "w");
    if (!grub_cfg) return false;

    fprintf(grub_cfg,
            "set timeout=5\n"
            "set default=0\n"
            "\n"
            "menuentry \"TouchOS\" {\n"
            "    insmod all_video\n"
            "    insmod gfxterm\n"
            "    terminal_output gfxterm\n"
            "    set root=(hd0,gpt2)\n"
            "    multiboot2 /boot/kernel.elf\n"
            "    boot\n"
            "}\n"
            "\n"
            "menuentry \"TouchOS (Safe Mode)\" {\n"
            "    set root=(hd0,gpt2)\n"
            "    multiboot2 /boot/kernel.elf safe\n"
            "    boot\n"
            "}\n");

    fclose(grub_cfg);

    return true;
}

void cleanup_mounts() {
    system("umount /mnt/touchos/boot/efi 2>/dev/null");
    system("umount /mnt/touchos 2>/dev/null");
    system("rmdir /mnt/touchos 2>/dev/null");
}

// ============================================================================
// Installation Thread (simulated as progress updates)
// ============================================================================

void* perform_installation(void* arg) {
    const char* device = ctx.disks[ctx.selected_disk].device;

    // Step 1: Partition disk
    ctx.progress_percent = 10;
    strcpy(ctx.progress_message, "Creating partitions...");
    if (!partition_disk(device)) {
        strcpy(ctx.error_message, "Failed to partition disk");
        ctx.installation_failed = true;
        return NULL;
    }

    // Step 2: Format partitions
    ctx.progress_percent = 30;
    strcpy(ctx.progress_message, "Formatting partitions...");
    if (!format_partitions(device)) {
        strcpy(ctx.error_message, "Failed to format partitions");
        ctx.installation_failed = true;
        return NULL;
    }

    // Step 3: Mount partitions
    ctx.progress_percent = 40;
    strcpy(ctx.progress_message, "Mounting filesystems...");
    if (!mount_partitions(device)) {
        strcpy(ctx.error_message, "Failed to mount partitions");
        ctx.installation_failed = true;
        return NULL;
    }

    // Step 4: Install system
    ctx.progress_percent = 50;
    strcpy(ctx.progress_message, "Installing system files...");
    if (!install_system()) {
        strcpy(ctx.error_message, "Failed to install system files");
        ctx.installation_failed = true;
        cleanup_mounts();
        return NULL;
    }

    // Step 5: Install bootloader
    ctx.progress_percent = 90;
    strcpy(ctx.progress_message, "Installing bootloader...");
    if (!install_bootloader(device)) {
        strcpy(ctx.error_message, "Failed to install bootloader");
        ctx.installation_failed = true;
        cleanup_mounts();
        return NULL;
    }

    // Step 6: Cleanup
    ctx.progress_percent = 100;
    strcpy(ctx.progress_message, "Installation complete!");
    cleanup_mounts();

    return NULL;
}

// ============================================================================
// UI Rendering
// ============================================================================

void render_welcome_screen() {
    touch_clear_screen(TOUCH_COLOR_BG);

    // Title
    touch_draw_text_centered(&(touch_rect_t){0, 100, TOUCH_SCREEN_WIDTH, 100},
                             "Welcome to TouchOS",
                             TOUCH_COLOR_TEXT, 48);

    // Subtitle
    touch_draw_text_centered(&(touch_rect_t){0, 200, TOUCH_SCREEN_WIDTH, 50},
                             "The Touch-Optimized Operating System",
                             TOUCH_COLOR_TEXT_DIM, 24);

    // Warning box
    touch_rect_t warning_box = {400, 350, 1120, 200};
    touch_draw_rounded_rect(warning_box.x, warning_box.y,
                           warning_box.width, warning_box.height,
                           10, TOUCH_COLOR_WARNING);

    touch_draw_text_centered(&(touch_rect_t){400, 370, 1120, 50},
                             "⚠️  WARNING",
                             TOUCH_COLOR_BG, 32);

    touch_draw_text_centered(&(touch_rect_t){400, 430, 1120, 100},
                             "This installer will erase all data on the selected disk.\n"
                             "Please backup your data before continuing.",
                             TOUCH_COLOR_BG, 20);

    // Continue button
    touch_button_draw(&ctx.buttons[0]);

    touch_flip_buffer();
}

void render_disk_select_screen() {
    touch_clear_screen(TOUCH_COLOR_BG);

    // Title
    touch_draw_text_centered(&(touch_rect_t){0, 50, TOUCH_SCREEN_WIDTH, 80},
                             "Select Installation Disk",
                             TOUCH_COLOR_TEXT, 36);

    // Instructions
    touch_draw_text_centered(&(touch_rect_t){0, 130, TOUCH_SCREEN_WIDTH, 40},
                             "Choose the disk where TouchOS will be installed",
                             TOUCH_COLOR_TEXT_DIM, 20);

    // Disk list
    int y = 200;
    for (int i = 0; i < ctx.disk_count; i++) {
        disk_info_t* disk = &ctx.disks[i];

        // Background
        uint32_t bg_color = (i == ctx.selected_disk) ?
                           TOUCH_COLOR_PRIMARY : TOUCH_COLOR_SURFACE;
        touch_draw_rounded_rect(300, y, 1320, 120, 10, bg_color);

        // Device name
        touch_draw_text(350, y + 25, disk->device, TOUCH_COLOR_TEXT, 28);

        // Model and size
        char info[256];
        snprintf(info, sizeof(info), "%s - %s", disk->model, disk->size);
        touch_draw_text(350, y + 65, info, TOUCH_COLOR_TEXT_DIM, 20);

        y += 140;
    }

    // Navigation buttons
    touch_button_draw(&ctx.buttons[0]); // Back
    touch_button_draw(&ctx.buttons[1]); // Continue

    touch_flip_buffer();
}

void render_partition_confirm_screen() {
    touch_clear_screen(TOUCH_COLOR_BG);

    disk_info_t* disk = &ctx.disks[ctx.selected_disk];

    // Title
    touch_draw_text_centered(&(touch_rect_t){0, 50, TOUCH_SCREEN_WIDTH, 80},
                             "Confirm Installation",
                             TOUCH_COLOR_TEXT, 36);

    // Selected disk info
    touch_rect_t disk_box = {400, 180, 1120, 150};
    touch_draw_rounded_rect(disk_box.x, disk_box.y,
                           disk_box.width, disk_box.height,
                           10, TOUCH_COLOR_SURFACE);

    touch_draw_text(450, 210, "Installation Disk:", TOUCH_COLOR_TEXT_DIM, 20);
    touch_draw_text(450, 250, disk->device, TOUCH_COLOR_TEXT, 32);

    char info[256];
    snprintf(info, sizeof(info), "%s (%s)", disk->model, disk->size);
    touch_draw_text(450, 290, info, TOUCH_COLOR_TEXT_DIM, 20);

    // Partition layout
    touch_draw_text(450, 380, "Partition Layout:", TOUCH_COLOR_TEXT_DIM, 20);

    touch_draw_rounded_rect(450, 420, 1020, 60, 5, TOUCH_COLOR_SECONDARY);
    touch_draw_text(470, 440, "EFI System Partition (512 MB)", TOUCH_COLOR_TEXT, 20);

    touch_draw_rounded_rect(450, 500, 1020, 60, 5, TOUCH_COLOR_SECONDARY);
    touch_draw_text(470, 520, "TouchOS Root (Remaining space)", TOUCH_COLOR_TEXT, 20);

    // Warning
    touch_rect_t warning = {400, 600, 1120, 100};
    touch_draw_rounded_rect(warning.x, warning.y,
                           warning.width, warning.height,
                           10, TOUCH_COLOR_ERROR);

    touch_draw_text_centered(&warning,
                             "⚠️  All data on this disk will be permanently erased!",
                             TOUCH_COLOR_TEXT, 22);

    // Navigation buttons
    touch_button_draw(&ctx.buttons[0]); // Back
    touch_button_draw(&ctx.buttons[1]); // Install Now

    touch_flip_buffer();
}

void render_installing_screen() {
    touch_clear_screen(TOUCH_COLOR_BG);

    // Title
    touch_draw_text_centered(&(touch_rect_t){0, 100, TOUCH_SCREEN_WIDTH, 80},
                             "Installing TouchOS",
                             TOUCH_COLOR_TEXT, 36);

    // Progress message
    touch_draw_text_centered(&(touch_rect_t){0, 200, TOUCH_SCREEN_WIDTH, 40},
                             ctx.progress_message,
                             TOUCH_COLOR_TEXT_DIM, 24);

    // Progress bar background
    touch_draw_rounded_rect(400, 300, 1120, 40, 20, TOUCH_COLOR_SURFACE);

    // Progress bar fill
    int fill_width = (1120 * ctx.progress_percent) / 100;
    if (fill_width > 0) {
        touch_draw_rounded_rect(400, 300, fill_width, 40, 20, TOUCH_COLOR_PRIMARY);
    }

    // Progress percentage
    char percent_text[16];
    snprintf(percent_text, sizeof(percent_text), "%d%%", ctx.progress_percent);
    touch_draw_text_centered(&(touch_rect_t){0, 360, TOUCH_SCREEN_WIDTH, 40},
                             percent_text,
                             TOUCH_COLOR_TEXT, 28);

    // Info text
    touch_draw_text_centered(&(touch_rect_t){0, 450, TOUCH_SCREEN_WIDTH, 60},
                             "Please do not turn off your computer",
                             TOUCH_COLOR_TEXT_DIM, 20);

    touch_flip_buffer();
}

void render_complete_screen() {
    touch_clear_screen(TOUCH_COLOR_BG);

    // Success icon (checkmark)
    touch_draw_text_centered(&(touch_rect_t){0, 100, TOUCH_SCREEN_WIDTH, 120},
                             "✓",
                             TOUCH_COLOR_SUCCESS, 128);

    // Title
    touch_draw_text_centered(&(touch_rect_t){0, 280, TOUCH_SCREEN_WIDTH, 80},
                             "Installation Complete!",
                             TOUCH_COLOR_TEXT, 36);

    // Message
    touch_draw_text_centered(&(touch_rect_t){0, 380, TOUCH_SCREEN_WIDTH, 60},
                             "TouchOS has been successfully installed on your computer.",
                             TOUCH_COLOR_TEXT_DIM, 22);

    touch_draw_text_centered(&(touch_rect_t){0, 450, TOUCH_SCREEN_WIDTH, 60},
                             "Please remove the installation media and restart.",
                             TOUCH_COLOR_TEXT_DIM, 22);

    // Reboot button
    touch_button_draw(&ctx.buttons[0]);

    touch_flip_buffer();
}

void render_error_screen() {
    touch_clear_screen(TOUCH_COLOR_BG);

    // Error icon
    touch_draw_text_centered(&(touch_rect_t){0, 100, TOUCH_SCREEN_WIDTH, 120},
                             "✗",
                             TOUCH_COLOR_ERROR, 128);

    // Title
    touch_draw_text_centered(&(touch_rect_t){0, 280, TOUCH_SCREEN_WIDTH, 80},
                             "Installation Failed",
                             TOUCH_COLOR_TEXT, 36);

    // Error message
    touch_draw_text_centered(&(touch_rect_t){0, 380, TOUCH_SCREEN_WIDTH, 60},
                             ctx.error_message,
                             TOUCH_COLOR_TEXT_DIM, 22);

    // Retry button
    touch_button_draw(&ctx.buttons[0]);

    touch_flip_buffer();
}

// ============================================================================
// Button Callbacks
// ============================================================================

void on_welcome_continue(void* data) {
    (void)data;
    ctx.state = INSTALLER_DISK_SELECT;
    detect_disks();
}

void on_disk_select_back(void* data) {
    (void)data;
    ctx.state = INSTALLER_WELCOME;
}

void on_disk_select_continue(void* data) {
    (void)data;
    if (ctx.selected_disk >= 0 && ctx.selected_disk < ctx.disk_count) {
        ctx.state = INSTALLER_PARTITION_CONFIRM;
    }
}

void on_partition_back(void* data) {
    (void)data;
    ctx.state = INSTALLER_DISK_SELECT;
}

void on_partition_install(void* data) {
    (void)data;
    ctx.state = INSTALLER_INSTALLING;
    ctx.progress_percent = 0;
    strcpy(ctx.progress_message, "Starting installation...");
    ctx.installation_failed = false;

    // Start installation (in real system, this would be a thread)
    perform_installation(NULL);

    if (ctx.installation_failed) {
        ctx.state = INSTALLER_ERROR;
    } else {
        ctx.state = INSTALLER_COMPLETE;
    }
}

void on_complete_reboot(void* data) {
    (void)data;
    printf("\nRebooting system...\n");
    system("reboot");
    exit(0);
}

void on_error_retry(void* data) {
    (void)data;
    ctx.state = INSTALLER_WELCOME;
}

// ============================================================================
// Event Handling
// ============================================================================

void handle_touch_event(touch_event_t* event) {
    if (event->type != TOUCH_EVENT_TAP) return;

    // Check button hits
    for (int i = 0; i < ctx.button_count; i++) {
        if (touch_button_hit_test(&ctx.buttons[i], event->x, event->y)) {
            if (ctx.buttons[i].on_tap) {
                ctx.buttons[i].on_tap(ctx.buttons[i].user_data);
                touch_sound_tap();
            }
            return;
        }
    }

    // Check disk selection
    if (ctx.state == INSTALLER_DISK_SELECT) {
        int y = 200;
        for (int i = 0; i < ctx.disk_count; i++) {
            if (event->y >= y && event->y < y + 120) {
                ctx.selected_disk = i;
                touch_sound_tap();
                return;
            }
            y += 140;
        }
    }
}

// ============================================================================
// UI Setup
// ============================================================================

void setup_ui_for_state() {
    ctx.button_count = 0;

    switch (ctx.state) {
        case INSTALLER_WELCOME:
            // Continue button
            ctx.buttons[0] = (touch_button_t){
                .bounds = {760, 650, 400, 80},
                .label = "Continue",
                .color = TOUCH_COLOR_PRIMARY,
                .enabled = true,
                .visible = true,
                .on_tap = on_welcome_continue,
                .user_data = NULL
            };
            ctx.button_count = 1;
            break;

        case INSTALLER_DISK_SELECT:
            // Back button
            ctx.buttons[0] = (touch_button_t){
                .bounds = {300, 900, 300, 80},
                .label = "Back",
                .color = TOUCH_COLOR_SECONDARY,
                .enabled = true,
                .visible = true,
                .on_tap = on_disk_select_back,
                .user_data = NULL
            };
            // Continue button
            ctx.buttons[1] = (touch_button_t){
                .bounds = {1320, 900, 300, 80},
                .label = "Continue",
                .color = TOUCH_COLOR_PRIMARY,
                .enabled = true,
                .visible = true,
                .on_tap = on_disk_select_continue,
                .user_data = NULL
            };
            ctx.button_count = 2;
            break;

        case INSTALLER_PARTITION_CONFIRM:
            // Back button
            ctx.buttons[0] = (touch_button_t){
                .bounds = {400, 750, 300, 80},
                .label = "Back",
                .color = TOUCH_COLOR_SECONDARY,
                .enabled = true,
                .visible = true,
                .on_tap = on_partition_back,
                .user_data = NULL
            };
            // Install Now button
            ctx.buttons[1] = (touch_button_t){
                .bounds = {1220, 750, 300, 80},
                .label = "Install Now",
                .color = TOUCH_COLOR_SUCCESS,
                .enabled = true,
                .visible = true,
                .on_tap = on_partition_install,
                .user_data = NULL
            };
            ctx.button_count = 2;
            break;

        case INSTALLER_COMPLETE:
            // Reboot button
            ctx.buttons[0] = (touch_button_t){
                .bounds = {760, 600, 400, 80},
                .label = "Restart Now",
                .color = TOUCH_COLOR_PRIMARY,
                .enabled = true,
                .visible = true,
                .on_tap = on_complete_reboot,
                .user_data = NULL
            };
            ctx.button_count = 1;
            break;

        case INSTALLER_ERROR:
            // Retry button
            ctx.buttons[0] = (touch_button_t){
                .bounds = {760, 550, 400, 80},
                .label = "Try Again",
                .color = TOUCH_COLOR_WARNING,
                .enabled = true,
                .visible = true,
                .on_tap = on_error_retry,
                .user_data = NULL
            };
            ctx.button_count = 1;
            break;

        default:
            break;
    }
}

// ============================================================================
// Main Loop
// ============================================================================

void installer_render() {
    switch (ctx.state) {
        case INSTALLER_WELCOME:
            render_welcome_screen();
            break;
        case INSTALLER_DISK_SELECT:
            render_disk_select_screen();
            break;
        case INSTALLER_PARTITION_CONFIRM:
            render_partition_confirm_screen();
            break;
        case INSTALLER_INSTALLING:
            render_installing_screen();
            break;
        case INSTALLER_COMPLETE:
            render_complete_screen();
            break;
        case INSTALLER_ERROR:
            render_error_screen();
            break;
    }
}

int main(int argc, char* argv[]) {
    printf("TouchOS Installer v1.0\n");
    printf("======================\n\n");

    // Check if running as root
    if (getuid() != 0) {
        printf("ERROR: Installer must be run as root.\n");
        printf("Please run: sudo %s\n", argv[0]);
        return 1;
    }

    // Initialize touch framework
    touch_framework_init();

    // Initialize installer state
    ctx.state = INSTALLER_WELCOME;
    ctx.selected_disk = -1;

    // Set up initial UI
    setup_ui_for_state();

    // Register event handler
    touch_register_event_handler(handle_touch_event);

    // Main loop
    installer_state_t last_state = ctx.state;
    while (1) {
        // Update UI if state changed
        if (ctx.state != last_state) {
            setup_ui_for_state();
            last_state = ctx.state;
        }

        // Render
        installer_render();

        // Handle events
        touch_event_t event;
        // In real implementation, this would poll for events

        // Sleep to avoid spinning
        touch_sleep_ms(16); // ~60 FPS

        // Exit on complete for now (in real system, wait for reboot)
        if (ctx.state == INSTALLER_COMPLETE) {
            touch_sleep_ms(5000);
            break;
        }
    }

    touch_framework_shutdown();
    return 0;
}
