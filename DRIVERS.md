# TouchOS Driver Documentation

← [Back to README](README.md) | [Kernel](KERNEL.md) | [Next: API Reference](API.md) →

## Overview

TouchOS includes custom device drivers for USB, touch input, graphics, and serial communication.

## Driver Architecture

```
Application Layer
        ↓
   libtouch API
        ↓
  /dev Interface
        ↓
  Driver Layer ← Hardware Interrupts
        ↓
   Hardware
```

## USB Subsystem

**Files**: `drivers/usb/`

### USB Controllers

#### XHCI (USB 3.0)
- **File**: `drivers/usb/xhci.c`
- PCI device detection
- Register mapping
- Transfer ring management
- Supports 5 Gbps (SuperSpeed)

#### EHCI (USB 2.0)
- Companion controller support
- 480 Mbps (High-Speed)
- Backward compatible with USB 1.1

#### UHCI (USB 1.1)
- Legacy support
- 12 Mbps (Full-Speed)

### USB Device Enumeration

1. Device plugged in
2. Controller detects device
3. Reset sequence
4. Get device descriptor
5. Assign address
6. Get configuration
7. Load appropriate driver

## Touch Input Drivers

**File**: `drivers/input/usb_touchscreen.c`

### Acer T230H Support

**Specifications**:
- Vendor ID: 0x0408
- Product ID: 0x3000
- Resolution: 4096×4096 (raw)
- Display: 1920×1080
- Max Contacts: 10-point multitouch

**Features**:
- HID multitouch protocol
- Automatic calibration
- Contact tracking
- Gesture support

**Calibration**:
```c
cal_x_min = 150
cal_x_max = 3946
cal_y_min = 130
cal_y_max = 3966

screen_x = ((raw_x - cal_x_min) * 1920) / (cal_x_max - cal_x_min)
screen_y = ((raw_y - cal_y_min) * 1080) / (cal_y_max - cal_y_min)
```

### Touch Event Flow

```
1. Touch detected by hardware
2. USB interrupt generated
3. XHCI driver handles interrupt
4. USB touchscreen driver receives HID report
5. Parse report (contact ID, x, y, pressure)
6. Apply calibration
7. Send event to /dev/input/eventX
8. libtouch reads event
9. Gesture recognition
10. Application callback
```

## Graphics Driver

**File**: `graphics/framebuffer.c`

### Framebuffer

- **Mode**: 1920×1080 32-bit ARGB
- **Memory**: Linear framebuffer
- **Access**: Direct pixel manipulation
- **Double Buffering**: Reduces tearing

**Setup**:
1. UEFI GOP or VESA VBE
2. Get framebuffer address
3. Map to virtual memory
4. Allocate back buffer

**Drawing**:
```c
// Set pixel
framebuffer[y * width + x] = color;

// Flip buffers
memcpy(front_buffer, back_buffer, fb_size);
```

## Serial Driver

**File**: `drivers/serial.c`

### COM Ports

- **COM1**: 0x3F8 (primary debug)
- **COM2**: 0x2F8 (secondary)

**Configuration**:
- Baud rate: 115200
- Data bits: 8
- Stop bits: 1
- Parity: None

**Functions**:
```c
void serial_init(void);
void serial_putchar(char c);
void serial_puts(const char* str);
```

**Usage**:
```c
serial_init();
serial_puts("Kernel initialized\n");
```

## Adding New Drivers

### 1. Create Driver File

```c
// drivers/mydevice/mydriver.c

#include "mydriver.h"

void mydriver_init(void) {
    // Initialize hardware
}

void mydriver_read(void* buffer, size_t size) {
    // Read from device
}

void mydriver_write(const void* buffer, size_t size) {
    // Write to device
}
```

### 2. Register with Kernel

```c
// In kernel_main()
mydriver_init();
```

### 3. Create Device Node

```c
// Create /dev/mydevice
device_register("mydevice", &mydriver_ops);
```

## Driver Development Tips

- **Use serial output for debugging**
- **Test in QEMU first**
- **Check PCI vendor/device IDs**
- **Read hardware datasheets**
- **Handle interrupts properly**

← [Back to README](README.md) | [Kernel](KERNEL.md) | [Next: API Reference](API.md) →
