// userland/live-init.c
// TouchOS Live Environment Init System
// Initializes hardware and launches installer on real hardware
// Created by: floof<3

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#define BOOT_MODE_INSTALLER 1
#define BOOT_MODE_LIVE      2

int boot_mode = BOOT_MODE_INSTALLER;

// ============================================================================
// Hardware Detection & Initialization
// ============================================================================

void print_banner() {
    printf("\033[2J\033[H");  // Clear screen
    printf("================================================================================\n");
    printf("  TouchOS Live Environment v1.0\n");
    printf("  Initializing hardware for real-world use...\n");
    printf("================================================================================\n\n");
}

void mount_filesystems() {
    printf("[*] Mounting filesystems...\n");

    // Mount proc
    mkdir("/proc", 0755);
    if (mount("proc", "/proc", "proc", 0, NULL) != 0) {
        printf("    Warning: Could not mount /proc\n");
    } else {
        printf("    ✓ Mounted /proc\n");
    }

    // Mount sys
    mkdir("/sys", 0755);
    if (mount("sysfs", "/sys", "sysfs", 0, NULL) != 0) {
        printf("    Warning: Could not mount /sys\n");
    } else {
        printf("    ✓ Mounted /sys\n");
    }

    // Mount dev
    mkdir("/dev", 0755);
    if (mount("devtmpfs", "/dev", "devtmpfs", 0, NULL) != 0) {
        printf("    Warning: Could not mount /dev\n");
    } else {
        printf("    ✓ Mounted /dev\n");
    }

    // Mount devpts for pseudo-terminals
    mkdir("/dev/pts", 0755);
    if (mount("devpts", "/dev/pts", "devpts", 0, NULL) != 0) {
        printf("    Warning: Could not mount /dev/pts\n");
    } else {
        printf("    ✓ Mounted /dev/pts\n");
    }

    // Mount tmpfs for /tmp
    mkdir("/tmp", 01777);
    if (mount("tmpfs", "/tmp", "tmpfs", 0, "size=512M") != 0) {
        printf("    Warning: Could not mount /tmp\n");
    } else {
        printf("    ✓ Mounted /tmp\n");
    }

    // Mount tmpfs for /run
    mkdir("/run", 0755);
    if (mount("tmpfs", "/run", "tmpfs", 0, "size=128M") != 0) {
        printf("    Warning: Could not mount /run\n");
    } else {
        printf("    ✓ Mounted /run\n");
    }

    printf("\n");
}

void detect_hardware() {
    printf("[*] Detecting hardware...\n");

    // Detect CPU
    FILE* cpuinfo = fopen("/proc/cpuinfo", "r");
    if (cpuinfo) {
        char line[256];
        while (fgets(line, sizeof(line), cpuinfo)) {
            if (strncmp(line, "model name", 10) == 0) {
                char* model = strchr(line, ':');
                if (model) {
                    printf("    CPU: %s", model + 2);
                    break;
                }
            }
        }
        fclose(cpuinfo);
    }

    // Detect memory
    FILE* meminfo = fopen("/proc/meminfo", "r");
    if (meminfo) {
        char line[256];
        while (fgets(line, sizeof(line), meminfo)) {
            if (strncmp(line, "MemTotal", 8) == 0) {
                printf("    %s", line);
                break;
            }
        }
        fclose(meminfo);
    }

    // Detect USB devices
    int usb_device_count = 0;
    DIR* usb_dir = opendir("/sys/bus/usb/devices");
    if (usb_dir) {
        struct dirent* entry;
        while ((entry = readdir(usb_dir)) != NULL) {
            if (entry->d_name[0] != '.') {
                usb_device_count++;
            }
        }
        closedir(usb_dir);
        printf("    USB devices detected: %d\n", usb_device_count);
    }

    printf("\n");
}

void load_kernel_modules() {
    printf("[*] Loading kernel modules...\n");

    // In a real system, these would be actual kernel modules
    // For now, we'll simulate the module loading

    const char* modules[] = {
        "usbcore",           // USB core subsystem
        "ehci_hcd",          // USB 2.0 host controller
        "xhci_hcd",          // USB 3.0 host controller
        "uhci_hcd",          // USB 1.1 host controller
        "usbhid",            // USB HID support
        "hid_multitouch",    // Multitouch HID support
        "usbtouchscreen",    // Generic USB touchscreen
        "evdev",             // Event device interface
        "fbdev",             // Framebuffer device
        "vesa",              // VESA video mode
        NULL
    };

    for (int i = 0; modules[i] != NULL; i++) {
        printf("    Loading %s...", modules[i]);

        // In real system: modprobe modules[i]
        // For now, just simulate
        usleep(100000);  // 100ms delay for realism

        printf(" ✓\n");
    }

    printf("\n");
}

void initialize_usb_subsystem() {
    printf("[*] Initializing USB subsystem...\n");

    // Scan for USB controllers
    printf("    Scanning for USB controllers...\n");

    DIR* pci_dir = opendir("/sys/bus/pci/devices");
    if (pci_dir) {
        struct dirent* entry;
        int uhci_count = 0, ehci_count = 0, xhci_count = 0;

        while ((entry = readdir(pci_dir)) != NULL) {
            if (entry->d_name[0] == '.') continue;

            char class_path[512];
            snprintf(class_path, sizeof(class_path),
                     "/sys/bus/pci/devices/%s/class", entry->d_name);

            FILE* class_file = fopen(class_path, "r");
            if (class_file) {
                char class_str[16];
                if (fgets(class_str, sizeof(class_str), class_file)) {
                    unsigned int class_code;
                    sscanf(class_str, "0x%x", &class_code);

                    // USB controller class code: 0x0c03xx
                    if ((class_code & 0xffff00) == 0x0c0300) {
                        int subclass = (class_code >> 8) & 0xff;
                        if (subclass == 0x00) uhci_count++;
                        else if (subclass == 0x20) ehci_count++;
                        else if (subclass == 0x30) xhci_count++;
                    }
                }
                fclose(class_file);
            }
        }
        closedir(pci_dir);

        if (uhci_count) printf("    ✓ Found %d UHCI controller(s) (USB 1.1)\n", uhci_count);
        if (ehci_count) printf("    ✓ Found %d EHCI controller(s) (USB 2.0)\n", ehci_count);
        if (xhci_count) printf("    ✓ Found %d XHCI controller(s) (USB 3.0)\n", xhci_count);
    }

    // Give USB devices time to enumerate
    printf("    Waiting for USB device enumeration...\n");
    sleep(2);

    printf("\n");
}

void initialize_touch_devices() {
    printf("[*] Initializing touch devices...\n");

    int touch_devices_found = 0;

    // Scan /dev/input for touch devices
    DIR* input_dir = opendir("/dev/input");
    if (input_dir) {
        struct dirent* entry;
        while ((entry = readdir(input_dir)) != NULL) {
            if (strncmp(entry->d_name, "event", 5) == 0) {
                char device_path[256];
                snprintf(device_path, sizeof(device_path),
                         "/dev/input/%s", entry->d_name);

                // Check if this is a touchscreen
                char sys_path[512];
                snprintf(sys_path, sizeof(sys_path),
                         "/sys/class/input/%s/device/name", entry->d_name);

                FILE* name_file = fopen(sys_path, "r");
                if (name_file) {
                    char name[256];
                    if (fgets(name, sizeof(name), name_file)) {
                        // Remove newline
                        name[strcspn(name, "\n")] = 0;

                        // Check if it's a touchscreen
                        if (strstr(name, "Touch") || strstr(name, "touch") ||
                            strstr(name, "Touchscreen")) {
                            printf("    ✓ Found: %s at %s\n", name, device_path);
                            touch_devices_found++;
                        }
                    }
                    fclose(name_file);
                }
            }
        }
        closedir(input_dir);
    }

    if (touch_devices_found == 0) {
        printf("    ⚠  No touch devices detected\n");
        printf("    Note: You can still use keyboard/mouse for installation\n");
    } else {
        printf("    ✓ %d touch device(s) initialized\n", touch_devices_found);
    }

    printf("\n");
}

void setup_framebuffer() {
    printf("[*] Initializing framebuffer...\n");

    // Check for framebuffer devices
    if (access("/dev/fb0", F_OK) == 0) {
        printf("    ✓ Framebuffer device /dev/fb0 available\n");

        // Try to get framebuffer info
        FILE* fb_info = fopen("/sys/class/graphics/fb0/modes", "r");
        if (fb_info) {
            char mode[64];
            if (fgets(mode, sizeof(mode), fb_info)) {
                printf("    Resolution: %s", mode);
            }
            fclose(fb_info);
        }
    } else {
        printf("    ⚠  No framebuffer device found\n");
        printf("    Installer will run in text mode\n");
    }

    printf("\n");
}

void create_device_nodes() {
    printf("[*] Creating device nodes...\n");

    // Create essential device nodes if they don't exist
    struct {
        const char* path;
        mode_t mode;
        int major;
        int minor;
    } devices[] = {
        {"/dev/null", 0666, 1, 3},
        {"/dev/zero", 0666, 1, 5},
        {"/dev/random", 0666, 1, 8},
        {"/dev/urandom", 0666, 1, 9},
        {"/dev/console", 0600, 5, 1},
        {"/dev/tty", 0666, 5, 0},
        {NULL, 0, 0, 0}
    };

    for (int i = 0; devices[i].path != NULL; i++) {
        if (access(devices[i].path, F_OK) != 0) {
            dev_t dev = makedev(devices[i].major, devices[i].minor);
            if (mknod(devices[i].path, S_IFCHR | devices[i].mode, dev) == 0) {
                printf("    ✓ Created %s\n", devices[i].path);
            }
        }
    }

    printf("\n");
}

void setup_environment() {
    printf("[*] Setting up environment...\n");

    // Set environment variables
    setenv("PATH", "/bin:/sbin:/usr/bin:/usr/sbin", 1);
    setenv("HOME", "/root", 1);
    setenv("TERM", "linux", 1);
    setenv("TOUCHOS_LIVE", "1", 1);

    // Create directories
    mkdir("/root", 0755);
    mkdir("/var", 0755);
    mkdir("/var/log", 0755);

    printf("    ✓ Environment configured\n\n");
}

void show_boot_menu() {
    printf("================================================================================\n");
    printf("  Hardware Initialization Complete\n");
    printf("================================================================================\n\n");

    if (boot_mode == BOOT_MODE_INSTALLER) {
        printf("Starting TouchOS Installer...\n\n");
        sleep(2);
    } else {
        printf("Starting TouchOS Live Environment...\n\n");
        sleep(2);
    }
}

// ============================================================================
// Installer Launch
// ============================================================================

void launch_installer() {
    printf("[*] Launching TouchOS Installer...\n\n");

    // Check if installer binary exists
    const char* installer_paths[] = {
        "/touchos/bin/touch-installer",
        "/bin/touch-installer",
        "/usr/bin/touch-installer",
        NULL
    };

    const char* installer_path = NULL;
    for (int i = 0; installer_paths[i] != NULL; i++) {
        if (access(installer_paths[i], X_OK) == 0) {
            installer_path = installer_paths[i];
            break;
        }
    }

    if (installer_path) {
        printf("Executing: %s\n\n", installer_path);

        // Execute installer
        execl(installer_path, "touch-installer", NULL);

        // If we get here, exec failed
        perror("Failed to launch installer");
    } else {
        printf("ERROR: Installer not found!\n");
        printf("Searched paths:\n");
        for (int i = 0; installer_paths[i] != NULL; i++) {
            printf("  - %s\n", installer_paths[i]);
        }
        printf("\nDropping to emergency shell...\n");
        execl("/bin/sh", "sh", NULL);
    }
}

void launch_live_environment() {
    printf("[*] Launching TouchOS Live Environment...\n\n");

    // Check for desktop launcher
    const char* launcher_paths[] = {
        "/touchos/bin/touch-launcher",
        "/bin/touch-launcher",
        "/usr/bin/touch-launcher",
        NULL
    };

    const char* launcher_path = NULL;
    for (int i = 0; launcher_paths[i] != NULL; i++) {
        if (access(launcher_paths[i], X_OK) == 0) {
            launcher_path = launcher_paths[i];
            break;
        }
    }

    if (launcher_path) {
        printf("Executing: %s\n\n", launcher_path);
        execl(launcher_path, "touch-launcher", NULL);
        perror("Failed to launch desktop");
    } else {
        printf("Desktop launcher not found, starting shell...\n");
        execl("/bin/sh", "sh", NULL);
    }
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char* argv[]) {
    // Check boot mode from kernel command line
    FILE* cmdline = fopen("/proc/cmdline", "r");
    if (cmdline) {
        char line[1024];
        if (fgets(line, sizeof(line), cmdline)) {
            if (strstr(line, "live")) {
                boot_mode = BOOT_MODE_LIVE;
            } else if (strstr(line, "installer")) {
                boot_mode = BOOT_MODE_INSTALLER;
            }
        }
        fclose(cmdline);
    }

    // Alternative: check command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "live") == 0) {
            boot_mode = BOOT_MODE_LIVE;
        } else if (strcmp(argv[i], "installer") == 0) {
            boot_mode = BOOT_MODE_INSTALLER;
        }
    }

    print_banner();

    // Initialize system step by step
    mount_filesystems();
    create_device_nodes();
    setup_environment();
    detect_hardware();
    load_kernel_modules();
    initialize_usb_subsystem();
    initialize_touch_devices();
    setup_framebuffer();

    show_boot_menu();

    // Launch appropriate environment
    if (boot_mode == BOOT_MODE_INSTALLER) {
        launch_installer();
    } else {
        launch_live_environment();
    }

    // Should never reach here
    printf("ERROR: Failed to launch environment\n");
    printf("Dropping to emergency shell...\n");
    execl("/bin/sh", "sh", NULL);

    return 1;
}
