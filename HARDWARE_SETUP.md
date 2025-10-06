# TouchOS Hardware Setup Guide

## 🖐️ Custom Touch System Configuration

TouchOS is built for unique custom hardware - a Dell Inspiron 13 7370 motherboard embedded inside an Acer T230H touchscreen monitor.

---

## 🔧 Hardware Specifications

### **Display: Acer T230H**
- **Type:** 23-inch LCD touchscreen monitor
- **Resolution:** 1920x1080 (Full HD)
- **Touch Technology:** Capacitive multi-touch
- **Touch Points:** 2 simultaneous touches
- **Touch Interface:** USB HID
- **USB Details:**
  - Vendor ID: `0x0408` (Quanta Computer)
  - Product ID: `0x3000`
- **Calibration Range:**
  - Raw X: 150 - 3946
  - Raw Y: 130 - 3966
  - Maps to: 1920x1080 screen

### **Computer: Dell Inspiron 13 7370 (Embedded)**

#### CPU
- **Model:** Intel Core i5-8250U or i7-8550U (8th gen)
- **Cores:** 4 cores / 8 threads
- **Base Clock:** 1.6-1.8 GHz
- **Turbo Boost:** Up to 4.0 GHz
- **Cache:** 6MB
- **TDP:** 15W

#### Memory
- **Type:** LPDDR3 1866MHz
- **Capacity:** 8GB or 16GB
- **Note:** Soldered to motherboard (non-upgradeable)

#### Graphics
- **GPU:** Intel UHD Graphics 620
- **Type:** Integrated (shared RAM)
- **Max Resolution:** 4096x2304 @ 60Hz
- **Video Outputs:** HDMI 1.4, USB-C (DisplayPort)
- **DirectX:** 12, OpenGL 4.5

#### Storage
- **Type:** NVMe SSD
- **Form Factor:** M.2 2280
- **Capacity:** 256GB or 512GB
- **Interface:** PCIe 3.0 x4

#### Networking
- **WiFi:** Intel Wireless-AC 8265
  - Standards: 802.11ac (up to 867 Mbps)
  - Dual-band: 2.4GHz + 5GHz
  - MU-MIMO support
- **Bluetooth:** 4.2
- **No Ethernet:** Wireless only

#### USB Ports (on original laptop)
- 2x USB 3.1 Gen 1 Type-A
- 1x USB 3.1 Gen 1 Type-C
- 1x USB 3.1 Gen 2 Type-C (with DisplayPort/Power Delivery)

#### Other Features
- **Audio:** Realtek ALC3246 (stereo speakers, combo jack)
- **Webcam:** HD (720p) - if preserved
- **Card Reader:** microSD

---

## 🔌 Physical Configuration

### **What's Different?**

**Removed (from original laptop):**
- ❌ Display panel (using external Acer monitor instead)
- ❌ Keyboard
- ❌ Touchpad
- ❌ Battery
- ❌ Laptop chassis/case

**Embedded Inside Monitor:**
- ✅ Motherboard
- ✅ CPU + cooling fan
- ✅ RAM
- ✅ SSD
- ✅ WiFi card

**Power:**
- Monitor powered by standard AC
- Motherboard powered by AC adapter (embedded)
- No battery = always AC-powered

---

## 💻 TouchOS Hardware Support

### **Fully Supported**

#### 1. **USB Touchscreen** ✅
File: `drivers/input/usb_touchscreen.c`

```c
// Acer T230H auto-detected by:
Vendor ID: 0x0408
Product ID: 0x3000

// Automatic calibration
Raw coordinates → Screen coordinates
Multi-touch: 2 points
```

**Features:**
- Automatic detection and initialization
- Pre-calibrated for 1920x1080
- Multi-touch gestures
- Touch pressure sensitivity
- Palm rejection (basic)

#### 2. **Intel UHD Graphics 620** ✅
File: `graphics/framebuffer.c`

```c
Resolution: 1920x1080
Color depth: 32-bit ARGB
Refresh rate: 60Hz
Double buffering: Enabled
```

**Features:**
- UEFI GOP framebuffer access
- Hardware acceleration ready
- Damage tracking for efficient updates
- Vsync support

#### 3. **Intel Wireless-AC 8265** 🔄
File: `kernel/net/network.c`

```c
Status: Driver stubs ready
WiFi: Needs firmware
Ethernet: None
```

**Current State:**
- Basic TCP/IP stack implemented
- WiFi driver needs Intel firmware
- Can use USB Ethernet adapter as alternative

#### 4. **NVMe SSD** ✅
Standard NVMe driver support

#### 5. **USB Controllers** ✅
File: `drivers/usb/xhci.c`

- XHCI (USB 3.x) driver implemented
- Enumerates USB devices
- Supports HID devices (touchscreen)

---

## 🚀 Boot Sequence

### 1. **UEFI Boot**
```
Power On → Dell UEFI BIOS → GRUB → TouchOS Bootloader
```

### 2. **Kernel Init**
```c
// kernel/kernel.c
kernel_main() {
    // 1. GDT/IDT setup
    // 2. Memory management (PMM + heap)
    // 3. Serial debug output
    // 4. USB controller init
    // 5. Touchscreen detection
    // 6. Graphics init
    // 7. Network init
    // 8. Window manager
}
```

### 3. **Hardware Detection**
```c
// kernel/touch_init.c
touch_system_init() {
    usb_init();              // Scan USB bus
    detect_touchscreen();    // Find 0x0408:0x3000
    calibrate_touch();       // Apply calibration
    graphics_init();         // Setup 1920x1080 FB
    wm_init();              // Start window manager
}
```

### 4. **Launch Package Manager**
```bash
# Auto-start on boot
/usr/bin/tpkg-touch-gui
```

---

## 🎮 Touch Interface

### **Gestures Supported**

**Single Touch:**
- Tap: Select/Click
- Drag: Move windows/scroll
- Long press: Right-click equivalent

**Two-Finger:**
- Pinch: Zoom in/out
- Two-finger drag: Scroll
- Two-finger tap: Context menu

### **On-Screen Keyboard**
- Automatically shows when text input needed
- QWERTY layout optimized for touch
- Large keys (70px height)
- Predictive text ready

### **Button Sizes**
- Minimum: 80px height (thumb-friendly)
- Spacing: 20px margins
- Corners: 10px rounded (easier to tap)

---

## ⚙️ Configuration Files

### **/etc/touchos.conf**
```ini
[Display]
resolution=1920x1080
refresh_rate=60
color_depth=32

[Touch]
device=Acer T230H
vendor_id=0x0408
product_id=0x3000
calibration=auto
multi_touch=enabled
max_points=2

[Hardware]
cpu=Intel Core i5-8250U
ram=8GB
storage=/dev/nvme0n1
wifi=Intel AC-8265

[Power]
mode=ac_only
battery=none
sleep=disabled
```

### **/etc/touch-calibration.conf**
```ini
[Calibration]
raw_x_min=150
raw_x_max=3946
raw_y_min=130
raw_y_max=3966
screen_width=1920
screen_height=1080
```

---

## 🔍 Troubleshooting

### **Touchscreen Not Detected**

```bash
# Check USB devices
lsusb | grep 0408:3000

# Should show:
# Bus 001 Device 003: ID 0408:3000 Quanta Computer, Inc.
```

**Fix:**
1. Replug USB cable
2. Check kernel messages: `dmesg | grep touch`
3. Verify driver loaded: `lsmod | grep usb_touch`

### **Touch Not Calibrated**

```bash
# Re-run calibration
/usr/bin/touch-calibrate

# Or manually edit
nano /etc/touch-calibration.conf
```

### **Graphics Issues**

```bash
# Check framebuffer
cat /sys/class/graphics/fb0/modes

# Should show: 1920x1080-60
```

### **WiFi Not Working**

```bash
# Intel WiFi needs firmware
cp iwlwifi-8265-*.ucode /lib/firmware/
modprobe iwlwifi
```

---

## 📊 Performance Tuning

### **CPU Governor**
```bash
# Performance mode (no battery to save)
echo performance > /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor
```

### **Graphics Acceleration**
```bash
# Enable Intel GPU acceleration
echo 1 > /sys/module/i915/parameters/enable_guc
```

### **Touch Responsiveness**
```bash
# Reduce touch polling interval
echo 1 > /sys/module/usb_hid/parameters/mousepoll
```

### **Thermal Management**
```bash
# Monitor temperatures (embedded = less airflow)
cat /sys/class/thermal/thermal_zone*/temp

# Fan control
echo 100 > /sys/class/hwmon/hwmon0/pwm1  # Full speed
```

---

## 🛠️ Hardware Modifications

### **What I Did:**
1. Removed Dell laptop components (screen, keyboard, etc.)
2. Embedded motherboard inside Acer monitor
3. Connected touchscreen USB to motherboard
4. Mounted cooling system
5. All-in-one touch computer!

### **Benefits:**
- ✅ Space-saving design
- ✅ Touch-first interface
- ✅ No external peripherals needed
- ✅ Clean setup
- ✅ Custom OS optimized for hardware

### **Challenges:**
- Thermal management (less airflow than laptop)
- No battery (always needs AC)
- Limited upgradability
- Custom mounting/support

---

## 📝 Driver Status

| Component | Driver | Status | File |
|-----------|--------|--------|------|
| Touchscreen | USB HID | ✅ Working | `drivers/input/usb_touchscreen.c` |
| Graphics | Intel UHD 620 | ✅ Working | `graphics/framebuffer.c` |
| USB | XHCI | ✅ Working | `drivers/usb/xhci.c` |
| WiFi | iwlwifi | 🔄 Needs firmware | `kernel/net/` |
| Audio | HDA | ⏳ TODO | - |
| NVMe | NVMe | ✅ Working | Standard driver |
| Webcam | UVC | ⏳ TODO | - |

---

## 🎯 Recommended Setup

### **1. Boot Configuration**
```bash
# GRUB config
GRUB_CMDLINE_LINUX="quiet splash touch_mode=1 i915.enable_guc=3"
```

### **2. Auto-Start Touch GUI**
```bash
# /etc/init.d/touchos
#!/bin/sh
/usr/bin/tpkg-touch-gui &
```

### **3. Disable Unused Services**
```bash
# No keyboard/mouse needed
systemctl disable gdm
systemctl disable bluetooth  # Unless using BT peripherals
```

---

## 🔐 Safety Considerations

### **Thermal:**
- Monitor inside monitor case
- Ensure adequate ventilation
- Consider additional cooling fan
- Thermal paste on CPU

### **Electrical:**
- Proper insulation of motherboard
- Ground monitoring power connections
- Use appropriate voltage regulators
- Protect from monitor's power supply

### **Physical:**
- Secure motherboard mounting
- Vibration dampening
- Cable management
- Easy access for maintenance

---

## 📚 Additional Resources

- **Intel UHD 620 Datasheet:** [intel.com/uhd620](https://intel.com)
- **Acer T230H Manual:** [acer.com/support](https://acer.com)
- **Dell Inspiron 7370 Service Manual:** [dell.com/support](https://dell.com)
- **USB HID Touch Spec:** [usb.org/hid](https://usb.org)

---

**Hardware Setup: Complete!** ✅


*Built for TouchOS by floof<3* 🖐️
