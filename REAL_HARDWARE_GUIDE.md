# TouchOS Real Hardware Deployment Guide

## Overview

This guide explains how to deploy TouchOS on real hardware, including USB boot, touch screen support, and 64-bit systems.

## Hardware Requirements

### Minimum Requirements

- **CPU**: 64-bit x86_64 processor (Intel Core 2 Duo or newer, AMD Athlon 64 or newer)
- **RAM**: 512 MB minimum, 2 GB recommended
- **Storage**: 5 GB minimum, 20 GB recommended
- **Boot**: UEFI or Legacy BIOS

### Recommended Hardware

- **CPU**: Intel Core i5/i7 or AMD Ryzen
- **RAM**: 4 GB or more
- **Storage**: SSD for best performance
- **Touch Screen**: USB HID-compliant touchscreen

### Tested Touch Devices

- **Acer T230H** (1920x1080, USB touch, 10-point multitouch) ✓ Fully supported
- **Generic USB HID Touchscreens** ✓ Should work
- **eGalax Touch Controllers** ✓ Compatible
- **3M MicroTouch** ✓ Compatible

## Creating Bootable USB Drive

### On Linux

#### Method 1: Using dd (Recommended)

```bash
# Find your USB device
lsblk

# Unmount if mounted
sudo umount /dev/sdX*

# Write ISO to USB (CAREFUL - this erases the USB drive!)
sudo dd if=touchos-installer.iso of=/dev/sdX bs=4M status=progress oflag=sync

# Verify write
sudo sync
```

Replace `/dev/sdX` with your USB device (e.g., `/dev/sdb`). **DO NOT use a partition like /dev/sdb1**, use the whole device `/dev/sdb`.

#### Method 2: Using Etcher (GUI)

1. Download [balenaEtcher](https://www.balena.io/etcher/)
2. Select `touchos-installer.iso`
3. Select your USB drive
4. Click "Flash!"

### On Windows

#### Using Rufus

1. Download [Rufus](https://rufus.ie/)
2. Insert USB drive
3. Select `touchos-installer.iso`
4. Settings:
   - Partition scheme: GPT or MBR (try GPT first)
   - Target system: UEFI or BIOS
5. Click "START"

### On macOS

```bash
# Find USB device
diskutil list

# Unmount
diskutil unmountDisk /dev/diskN

# Write ISO
sudo dd if=touchos-installer.iso of=/dev/rdiskN bs=4m

# Eject
diskutil eject /dev/diskN
```

## Booting from USB

### UEFI Systems

1. Insert USB drive
2. Restart computer
3. Enter BIOS/UEFI (usually F2, F10, F12, or DEL)
4. Go to Boot menu
5. Select USB drive (may be listed as "UEFI: USB Drive" or "TouchOS")
6. Save and exit

**Quick Boot Menu**: On most systems, pressing F12 or F11 during startup opens a boot device selection menu.

### Legacy BIOS Systems

1. Insert USB drive
2. Restart computer
3. Enter BIOS (usually DEL or F2)
4. Go to Boot order
5. Move USB drive to top of boot priority
6. Save and exit

### Common Boot Keys by Manufacturer

| Manufacturer | BIOS Key | Boot Menu |
|--------------|----------|-----------|
| Acer         | F2, DEL  | F12       |
| ASUS         | F2, DEL  | F8        |
| Dell         | F2       | F12       |
| HP           | F10, ESC | F9        |
| Lenovo       | F2, F1   | F12       |
| MSI          | DEL      | F11       |
| Samsung      | F2       | F12       |
| Toshiba      | F2       | F12       |

## Installation Process

### Step 1: Boot Menu

When you boot from USB, you'll see:

```
================================================================================
  TouchOS Boot Menu
================================================================================

1. Install TouchOS
2. Install TouchOS (Safe Mode - No Graphics)
3. Try TouchOS (Live Mode)
4. Boot from First Hard Drive
5. Reboot
6. Shutdown
```

Select **Install TouchOS** for normal installation with touch support.

### Step 2: Hardware Detection

The system will automatically:

```
[*] Mounting filesystems...
    ✓ Mounted /proc
    ✓ Mounted /sys
    ✓ Mounted /dev

[*] Detecting hardware...
    CPU: Intel(R) Core(TM) i7-4770 CPU @ 3.40GHz
    MemTotal: 16384 MB
    USB devices detected: 8

[*] Loading kernel modules...
    Loading usbcore... ✓
    Loading ehci_hcd... ✓
    Loading xhci_hcd... ✓
    Loading usbhid... ✓
    Loading hid_multitouch... ✓
    Loading usbtouchscreen... ✓

[*] Initializing USB subsystem...
    ✓ Found 2 EHCI controller(s) (USB 2.0)
    ✓ Found 1 XHCI controller(s) (USB 3.0)

[*] Initializing touch devices...
    ✓ Found: Acer T230H Touchscreen at /dev/input/event4
    ✓ 1 touch device(s) initialized

[*] Initializing framebuffer...
    ✓ Framebuffer device /dev/fb0 available
    Resolution: 1920x1080
```

### Step 3: Touch-Based Installation

The graphical installer will launch with full touch support:

1. **Welcome Screen** - Touch "Continue"
2. **Disk Selection** - Tap your target disk
3. **Confirmation** - Review and tap "Install Now"
4. **Installation** - Watch progress bar
5. **Complete** - Remove USB and tap "Restart Now"

### Step 4: First Boot

After installation:

1. Remove USB drive
2. System will reboot
3. TouchOS will boot from your hard drive
4. Touch-based desktop will load

## Touch Screen Calibration

### Automatic Calibration

TouchOS auto-calibrates for common touchscreens. For Acer T230H, calibration is pre-configured.

### Manual Calibration

If touch points are off:

1. Open Settings app
2. Go to "Display & Touch"
3. Select "Calibrate Touch Screen"
4. Follow on-screen instructions

### Adding Custom Touch Device

If your touchscreen isn't recognized, you can add support:

1. Edit `/etc/touchos/touch-devices.conf`
2. Add your device:

```
[MyTouchScreen]
vendor_id = 0x0408
product_id = 0x3000
max_x = 4096
max_y = 4096
cal_x_min = 150
cal_x_max = 3946
cal_y_min = 130
cal_y_max = 3966
```

3. Restart touch service

## Troubleshooting

### USB Won't Boot

**Problem**: BIOS doesn't see USB drive

**Solutions**:
- Try different USB port (USB 2.0 ports work more reliably)
- Recreate USB drive using different tool
- Try MBR instead of GPT in Rufus
- Enable "USB Boot" in BIOS
- Disable "Secure Boot" in UEFI

### No Touch Input

**Problem**: Touch screen not working

**Solutions**:

1. **Check USB Connection**
   ```bash
   lsusb | grep -i touch
   ```

2. **Check Touch Driver**
   ```bash
   cat /proc/bus/input/devices | grep -A 10 Touch
   ```

3. **Test Raw Input**
   ```bash
   evtest /dev/input/event4
   ```

4. **Use Safe Mode**
   - Boot with "Install TouchOS (Safe Mode)"
   - Use keyboard/mouse for installation

### Black Screen After Boot

**Problem**: Graphics not initializing

**Solutions**:
- Boot in Safe Mode
- Add kernel parameter: `nomodeset`
- Add kernel parameter: `vga=791` (1024x768)
- Update BIOS/UEFI firmware

### Installer Shows "No disks found"

**Problem**: Can't see hard drives

**Solutions**:
- Switch SATA mode in BIOS (AHCI vs IDE)
- Check disk is connected
- Try different SATA port
- Update BIOS

### Touch Input Inverted or Rotated

**Problem**: Touch coordinates are wrong

**Solutions**:
- Edit `/etc/touchos/touch.conf`
- Set `invert_x = true` or `invert_y = true`
- Set `swap_xy = true` for rotated screens

## Hardware-Specific Notes

### Acer T230H (Tested & Recommended)

- **Resolution**: 1920x1080
- **Touch**: 10-point multitouch via USB
- **Status**: ✓ Fully supported out of the box
- **Notes**: Pre-calibrated, no configuration needed

### Dell Touch Monitors

- Most USB HID touch monitors work automatically
- May need manual calibration
- Use evtest to find correct event device

### HP Touch Displays

- Usually detected as HID touchscreen
- Works with generic driver
- Good multitouch support

### Generic USB Touchscreens

- If it's USB HID-compliant, it should work
- May need calibration
- Test with evtest first

## Performance Optimization for Real Hardware

### For SSDs

Add to `/etc/fstab`:
```
/dev/sda2 / ext4 discard,noatime 0 1
```

### For Touch Responsiveness

In `/etc/touchos/touch.conf`:
```
touch_poll_rate = 125  # Hz (125 = 8ms latency)
touch_threshold = 10   # Pixels
```

### For Low RAM Systems

Edit `/etc/touchos/system.conf`:
```
enable_swap = true
swap_size = 2G
```

## Dual Boot Configuration

TouchOS can coexist with Windows or Linux.

### With Windows

1. Install Windows first
2. Leave unpartitioned space for TouchOS
3. Install TouchOS to free space
4. GRUB will detect Windows automatically

### With Linux

1. Install both operating systems
2. TouchOS's GRUB will detect other Linux distros
3. Or run `sudo update-grub` from your Linux distro

### Boot Menu

GRUB shows all installed operating systems:
- TouchOS
- Windows Boot Manager
- Ubuntu (or other Linux)

## Advanced Configuration

### Kernel Parameters

Edit `/boot/grub/grub.cfg` and add to `linux` line:

- `nomodeset` - Disable kernel mode setting (for GPU issues)
- `acpi=off` - Disable ACPI (for old hardware)
- `noapic` - Disable APIC (for IRQ issues)
- `vga=791` - Force 1024x768 resolution
- `touchos_debug=1` - Enable debug mode

### Custom Resolution

Add to grub.cfg:
```
set gfxmode=1920x1080
set gfxpayload=keep
```

## Getting Help

### Check Logs

```bash
dmesg | grep -i touch    # Touch driver messages
dmesg | grep -i usb      # USB messages
dmesg | grep -i error    # Error messages
```

### Test Touch Input

```bash
evtest /dev/input/event4
```
Tap screen and see if events are generated.

### System Information

```bash
uname -a                 # Kernel version
lsusb                    # USB devices
lspci                    # PCI devices
cat /proc/cpuinfo        # CPU info
free -h                  # Memory info
```

## Frequently Asked Questions

### Q: Can I install on a tablet?

**A**: Yes, if it's x86_64 based. ARM tablets are not supported.

### Q: Will it work with external USB touchscreens?

**A**: Yes, plug in before booting or it will hot-plug.

### Q: Can I use mouse and keyboard instead of touch?

**A**: Yes, all touch interfaces work with mouse/keyboard too.

### Q: What about touchpad gestures?

**A**: Touchpad is supported as a pointing device. Multitouch gestures depend on hardware.

### Q: Can I run this in VirtualBox with USB passthrough?

**A**: Yes, enable USB 3.0 controller and pass through your touch device.

### Q: Does it work on Apple hardware?

**A**: Intel Macs: Yes (use Boot Camp or rEFInd)
         M1/M2 Macs: No (ARM architecture not supported)

## System Requirements Summary

| Component    | Minimum        | Recommended      |
|--------------|----------------|------------------|
| CPU          | x86_64 64-bit  | Multi-core       |
| RAM          | 512 MB         | 2+ GB            |
| Storage      | 5 GB           | 20+ GB SSD       |
| Graphics     | VGA            | 1920x1080        |
| Touch        | Optional       | USB HID          |
| Boot         | BIOS/UEFI      | UEFI             |

## Support

For hardware compatibility issues:
1. Check logs: `dmesg`
2. Test in Safe Mode
3. Report issues with hardware details

---

**Ready to install?** Create your USB drive and boot from it! The installer will guide you through the rest.
