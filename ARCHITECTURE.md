# TouchOS Architecture

← [Back to README](README.md) | [Next: Kernel Documentation](KERNEL.md) →

## Overview

TouchOS is a custom 64-bit operating system designed from scratch for touch-screen interfaces, specifically optimized for the Acer T230H touchscreen display with embedded Dell Inspiron 13 7370 hardware.

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────────────┐
│                        TouchOS Architecture                          │
├─────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                   USER SPACE (Ring 3)                        │   │
│  │                                                               │   │
│  │  ┌──────────────────────────────────────────────────────┐  │   │
│  │  │  Touch Applications                                   │  │   │
│  │  │  • Launcher      • File Manager    • Settings        │  │   │
│  │  │  • Terminal      • System Monitor  • Installer       │  │   │
│  │  └──────────────────────────────────────────────────────┘  │   │
│  │                          ↕                                  │   │
│  │  ┌──────────────────────────────────────────────────────┐  │   │
│  │  │  libtouch Framework                                   │  │   │
│  │  │  • Touch Event Handling  • UI Widgets                │  │   │
│  │  │  • Gesture Recognition   • Drawing API               │  │   │
│  │  │  • On-Screen Keyboard    • Window Management         │  │   │
│  │  └──────────────────────────────────────────────────────┘  │   │
│  │                          ↕                                  │   │
│  │  ┌──────────────────────────────────────────────────────┐  │   │
│  │  │  Package Manager (tpkg)                               │  │   │
│  │  │  • CLI Tool      • Package Builder    • Touch GUI    │  │   │
│  │  └──────────────────────────────────────────────────────┘  │   │
│  └───────────────────────────────────────────────────────────┘   │
│                                                                       │
├───────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                   KERNEL SPACE (Ring 0)                      │   │
│  │                                                               │   │
│  │  ┌──────────────────────────────────────────────────────┐  │   │
│  │  │  System Call Interface                                │  │   │
│  │  └──────────────────────────────────────────────────────┘  │   │
│  │                          ↕                                  │   │
│  │  ┌──────────────────────────────────────────────────────┐  │   │
│  │  │  TouchOS Kernel (64-bit)                             │  │   │
│  │  │  • Process Scheduler    • Memory Manager (PMM+Heap)  │  │   │
│  │  │  • Interrupt Handler    • System Calls               │  │   │
│  │  │  • VFS                  • IPC                         │  │   │
│  │  └──────────────────────────────────────────────────────┘  │   │
│  │                          ↕                                  │   │
│  │  ┌──────────────────────────────────────────────────────┐  │   │
│  │  │  Device Drivers                                       │  │   │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────────────┐   │  │   │
│  │  │  │   USB    │  │  Touch   │  │   Framebuffer   │   │  │   │
│  │  │  │  Stack   │  │  Input   │  │    Graphics     │   │  │   │
│  │  │  │          │  │          │  │                  │   │  │   │
│  │  │  │ XHCI     │  │ USB HID  │  │  VESA/GOP       │   │  │   │
│  │  │  │ EHCI     │  │ Acer     │  │  Double-buffer  │   │  │   │
│  │  │  │ UHCI     │  │ T230H    │  │  1920x1080      │   │  │   │
│  │  │  └──────────┘  └──────────┘  └──────────────────┘   │  │   │
│  │  │  ┌──────────┐  ┌──────────┐  ┌──────────────────┐   │  │   │
│  │  │  │  Serial  │  │ Network  │  │   Storage       │   │  │   │
│  │  │  │  COM1/2  │  │  (WIP)   │  │   SATA/NVMe     │   │  │   │
│  │  │  └──────────┘  └──────────┘  └──────────────────┘   │  │   │
│  │  └──────────────────────────────────────────────────────┘  │   │
│  └───────────────────────────────────────────────────────────┘   │
│                                                                       │
├───────────────────────────────────────────────────────────────────────┤
│                                                                       │
│  ┌─────────────────────────────────────────────────────────────┐   │
│  │                   BOOTLOADER (16/32/64-bit)                  │   │
│  │                                                               │   │
│  │  ┌──────────────────────────────────────────────────────┐  │   │
│  │  │  GRUB2 (Multiboot2)                                   │  │   │
│  │  │  • UEFI Support          • Legacy BIOS Support        │  │   │
│  │  │  • 64-bit Mode Switch    • Memory Map                 │  │   │
│  │  └──────────────────────────────────────────────────────┘  │   │
│  └───────────────────────────────────────────────────────────┘   │
│                                                                       │
└───────────────────────────────────────────────────────────────────────┘
                                 ↕
┌───────────────────────────────────────────────────────────────────────┐
│                          HARDWARE LAYER                                │
│                                                                         │
│  CPU: Intel i5-8250U / i7-8550U (x86_64, 4 cores, 8 threads)          │
│  RAM: 8-16 GB DDR4                                                     │
│  Storage: NVMe SSD                                                     │
│  Graphics: Intel UHD 620                                               │
│  Display: Acer T230H (1920x1080, 10-point multitouch via USB)        │
│  USB: 3.0 + 2.0 Controllers                                           │
└───────────────────────────────────────────────────────────────────────┘
```

## Core Components

### 1. Bootloader Layer

**GRUB2 with Multiboot2 Protocol**
- Handles 16-bit → 32-bit → 64-bit mode transition
- Sets up initial page tables
- Loads kernel into memory
- Passes memory map and boot info to kernel
- Supports both UEFI and Legacy BIOS

**Boot Process:**
1. BIOS/UEFI firmware loads GRUB2
2. GRUB2 displays boot menu
3. User selects TouchOS
4. GRUB2 loads `kernel.elf` into memory
5. Executes `boot64.asm` (32-bit entry point)
6. Switches to long mode (64-bit)
7. Jumps to `kernel_main()`

### 2. Kernel Layer

**64-bit Microkernel Architecture**

**Core Subsystems:**

#### Memory Management
- **Physical Memory Manager (PMM)**: Manages physical RAM pages
  - Bitmap allocator
  - 4KB page granularity
  - Identity mapping of first 512 MB
- **Heap Allocator**: Kernel dynamic memory
  - `kmalloc()` / `kfree()`
  - Block-based allocation
  - Metadata tracking

#### Process Management
- Scheduler (cooperative/preemptive)
- Thread management
- Context switching
- Process creation/termination

#### Interrupt Handling
- IDT (Interrupt Descriptor Table) setup
- ISR (Interrupt Service Routines)
- IRQ handling
- Timer interrupts

#### System Calls
- System call interface
- User-kernel communication
- Protected mode transitions

**See [KERNEL.md](KERNEL.md) for detailed kernel documentation.**

### 3. Driver Layer

TouchOS includes drivers for essential hardware:

#### USB Subsystem
- **XHCI Driver** - USB 3.0 support
- **EHCI Driver** - USB 2.0 support
- **UHCI Driver** - USB 1.1 support
- **USB Hub** - Device enumeration
- **HID Class** - Human Interface Devices

#### Input Drivers
- **USB Touchscreen Driver**
  - Acer T230H specific support
  - HID multitouch protocol
  - 10-point touch tracking
  - Automatic calibration for 1920x1080
- **USB Keyboard/Mouse** - Standard HID input

#### Graphics Drivers
- **Framebuffer Driver**
  - VESA/GOP mode setting
  - 1920x1080 32-bit color
  - Double buffering
  - Direct pixel access

#### Other Drivers
- **Serial Driver** - COM1/COM2 for debugging
- **Storage Drivers** - SATA, NVMe (planned)
- **Network Drivers** - WiFi/Ethernet (planned)

**See [DRIVERS.md](DRIVERS.md) for detailed driver documentation.**

### 4. User Space Layer

#### libtouch Framework

The core UI framework providing:

**Touch Event System:**
- Multi-touch support (up to 10 points)
- Gesture recognition (tap, swipe, pinch, rotate)
- Event callbacks

**UI Widgets:**
- Buttons, lists, text inputs
- Sliders, switches
- Windows, dialogs

**Drawing API:**
- Shapes, text, icons
- Double-buffered rendering
- Color management
- Effects (shadows, gradients)

**On-Screen Keyboard:**
- QWERTY/Numeric/Symbols layouts
- Touch-optimized key sizes
- Auto-show for text inputs

**See [API.md](API.md) for complete API reference.**

#### Applications

**Core Apps:**
- **Touch Launcher** - Desktop with app grid
- **File Manager** - Browse and manage files
- **Settings** - System configuration
- **Terminal** - Command-line interface
- **System Monitor** - Resource monitoring

**System Apps:**
- **Touch Installer** - OS installation wizard
- **Package Manager GUI** - Visual package management

**See [TOUCH_APPS_COMPLETE.md](TOUCH_APPS_COMPLETE.md) for app details.**

#### Package Manager (tpkg)

**Three Components:**

1. **tpkg** - CLI package manager
   - Install/remove packages
   - Dependency resolution
   - Repository management

2. **tpkg-build** - Package creation tool
   - Build .tpkg files
   - Upload to repository
   - Verification

3. **tpkg-touch-gui** - Touch interface
   - Visual package browsing
   - One-touch installation
   - Update management

**See [PACKAGE_MANAGER.md](PACKAGE_MANAGER.md) for details.**

## Data Flow Examples

### Touch Input Flow

```
1. User touches screen
   ↓
2. USB Touchscreen hardware detects touch
   ↓
3. USB controller generates interrupt
   ↓
4. XHCI/EHCI driver handles interrupt
   ↓
5. USB Touchscreen Driver receives HID report
   ↓
6. Driver calibrates coordinates (raw → 1920x1080)
   ↓
7. Input event sent to /dev/input/eventX
   ↓
8. libtouch reads event
   ↓
9. Gesture recognition (tap/swipe/etc.)
   ↓
10. Application callback invoked
    ↓
11. App updates UI
    ↓
12. libtouch renders to framebuffer
    ↓
13. Display shows result
```

### System Call Flow

```
1. App calls touch_draw_rect()
   ↓
2. libtouch prepares draw command
   ↓
3. Writes to framebuffer memory
   ↓
4. (For privileged operations)
   ↓
5. syscall instruction (int 0x80)
   ↓
6. CPU switches to kernel mode
   ↓
7. Kernel system call handler
   ↓
8. Kernel performs operation
   ↓
9. Returns to user mode
   ↓
10. App continues execution
```

### Package Installation Flow

```
1. User taps "Install" in tpkg-touch-gui
   ↓
2. GUI calls tpkg CLI
   ↓
3. tpkg downloads .tpkg file
   ↓
4. Verifies signature
   ↓
5. Extracts to /tmp
   ↓
6. Checks dependencies
   ↓
7. Runs install script
   ↓
8. Copies files to /usr/bin, /usr/lib
   ↓
9. Updates package database
   ↓
10. Notifies GUI of completion
    ↓
11. GUI shows success message
```

## Memory Layout

```
Virtual Address Space (64-bit):

0x0000_0000_0000_0000  ┌─────────────────────────┐
                       │  User Space             │
                       │  (Applications)         │
                       │                         │
0x0000_7FFF_FFFF_FFFF  ├─────────────────────────┤
                       │  (Invalid - Guard)      │
0xFFFF_8000_0000_0000  ├─────────────────────────┤
                       │  Kernel Space           │
                       │  • Kernel Code          │
                       │  • Kernel Data          │
                       │  • Kernel Heap          │
                       │  • Device Memory        │
                       │  • Page Tables          │
0xFFFF_FFFF_FFFF_FFFF  └─────────────────────────┘

Physical Memory Map (example 16 GB system):

0x0000_0000  ┌─────────────────────────┐
             │  BIOS/Firmware          │
0x0010_0000  ├─────────────────────────┤
             │  Kernel Image           │
0x0040_0000  ├─────────────────────────┤
             │  PMM Bitmap             │
0x0050_0000  ├─────────────────────────┤
             │  Kernel Heap            │
0x0100_0000  ├─────────────────────────┤
             │  User Space Memory      │
             │  (Dynamically allocated)│
             │                         │
0x4000_0000  ├─────────────────────────┤  (16 GB)
             │  Reserved/Devices       │
0xFFFF_FFFF  └─────────────────────────┘
```

## Build System

```
Source Files
    ↓
┌───────────────────────────────────┐
│  Makefile (root)                  │
│  • Compiles kernel (gcc -m64)    │
│  • Assembles bootloader (nasm)   │
│  • Links with linker script      │
│  • Builds userland               │
└───────────────────────────────────┘
    ↓
    ├── kernel.elf (kernel binary)
    ├── userland apps (executables)
    └── libtouch.a (static library)
    ↓
┌───────────────────────────────────┐
│  build-iso.sh                     │
│  • Creates ISO directory          │
│  • Copies kernel + apps           │
│  • Generates GRUB config          │
│  • Builds bootable ISO            │
└───────────────────────────────────┘
    ↓
touchos-installer.iso (22 MB)
```

**See [BUILDING.md](BUILDING.md) for build instructions.**

## Performance Characteristics

| Component | Performance |
|-----------|-------------|
| Boot Time | ~5-10 seconds (GRUB → Desktop) |
| Touch Latency | <10ms (hardware to app callback) |
| Frame Rate | 60 FPS (with vsync) |
| Memory Overhead | ~50 MB (kernel + core services) |
| Disk I/O | Limited by hardware (NVMe: ~3 GB/s) |

## Design Philosophy

### 1. **Touch-First**
- Every UI element designed for finger input
- Minimum 44px touch targets
- Large, clear buttons (80px height)
- Gestures over keyboard shortcuts

### 2. **Simplicity**
- Microkernel approach
- Clear separation of concerns
- Minimal dependencies

### 3. **Performance**
- Direct framebuffer access (no X11)
- Double-buffered rendering
- Optimized for specific hardware

### 4. **Customization**
- Built for Acer T230H + Dell Inspiron 13 7370
- Optimized for this exact configuration
- Maximum performance for this hardware

## Technology Stack

| Layer | Technology |
|-------|------------|
| Language | C (kernel + drivers + userland) |
| Assembly | NASM (x86_64 boot code) |
| Bootloader | GRUB2 (Multiboot2) |
| Build System | GNU Make + Shell Scripts |
| Graphics | Framebuffer (VESA/GOP) |
| Touch Input | USB HID Multitouch |
| Packaging | Custom .tpkg format |

## Security Model

- Ring 0 (Kernel) vs Ring 3 (User Space) separation
- Memory protection (paging)
- System call validation
- (Future: Process isolation, sandboxing)

## Future Architecture Enhancements

- [ ] Network stack (TCP/IP)
- [ ] Multi-process scheduler
- [ ] Virtual file system (VFS)
- [ ] Sound subsystem
- [ ] GPU acceleration
- [ ] Power management

---

## Related Documentation

- **[Kernel Documentation](KERNEL.md)** - Detailed kernel internals
- **[Driver Documentation](DRIVERS.md)** - Driver architecture
- **[API Reference](API.md)** - libtouch framework API
- **[Building Guide](BUILDING.md)** - How to build TouchOS
- **[Development Guide](DEVELOPMENT.md)** - Contributing to TouchOS

← [Back to README](README.md) | [Next: Kernel Documentation](KERNEL.md) →
