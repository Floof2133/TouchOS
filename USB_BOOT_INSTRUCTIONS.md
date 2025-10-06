# TouchOS USB Boot Instructions - Quick Start

## What You Need

**touchos-installer.iso** - A 22MB bootable ISO that works on:
- ✅ USB drives (dd to USB for bootable installer)
- ✅ CD/DVD (burn directly)
- ✅ 64-bit x86_64 systems
- ✅ UEFI and Legacy BIOS
- ✅ Real hardware with touch support

## Quick Start: Create USB Installer

### On Linux (Fastest Method)

```bash
# 1. Find your USB drive
lsblk

# 2. Unmount it (replace sdX with your device, e.g., sdb)
sudo umount /dev/sdX*

# 3. Write ISO to USB
sudo dd if=touchos-installer.iso of=/dev/sdX bs=4M status=progress oflag=sync

# Done! USB is bootable.
```

**⚠️ WARNING**: `dd` will ERASE everything on the USB drive!

### On Windows

1. Download **Rufus** from https://rufus.ie/
2. Select `touchos-installer.iso`
3. Select your USB drive
4. Click "START"
5. Done!

### On macOS

```bash
# 1. Find USB device
diskutil list

# 2. Unmount (replace diskN with your device)
diskutil unmountDisk /dev/diskN

# 3. Write ISO
sudo dd if=touchos-installer.iso of=/dev/rdiskN bs=4m

# 4. Eject
diskutil eject /dev/diskN
```

## Boot from USB

1. **Insert USB drive**
2. **Restart computer**
3. **Press boot menu key** (usually F12, F11, or ESC during startup)
4. **Select USB drive** from boot menu
5. **Wait for TouchOS menu** to appear

### Can't Find Boot Menu Key?

| Manufacturer | Boot Menu Key |
|--------------|---------------|
| Acer         | F12           |
| ASUS         | F8            |
| Dell         | F12           |
| HP           | F9, ESC       |
| Lenovo       | F12           |
| MSI          | F11           |
| Samsung      | F12           |

Or enter BIOS (F2/DEL) and change boot order.

## What Happens When You Boot

### 1. Boot Menu Appears

```
================================================================================
  TouchOS Boot Menu
================================================================================

1. Install TouchOS                          ← Choose this!
2. Install TouchOS (Safe Mode - No Graphics)
3. Try TouchOS (Live Mode)
4. Boot from First Hard Drive
5. Reboot
6. Shutdown
```

### 2. Hardware Detection (Automatic)

The system will automatically detect and initialize:

```
[*] Mounting filesystems...
    ✓ Mounted /proc, /sys, /dev

[*] Detecting hardware...
    CPU: Intel(R) Core(TM) i7-4770 CPU @ 3.40GHz
    MemTotal: 16384 MB

[*] Loading kernel modules...
    Loading usbcore... ✓
    Loading ehci_hcd... ✓     (USB 2.0)
    Loading xhci_hcd... ✓     (USB 3.0)
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

### 3. Touch Installer Launches

The graphical touch installer will start automatically!

You can now use:
- **Touch screen** (if detected)
- **Mouse**
- **Keyboard**

## Installation Steps

1. **Welcome Screen**
   - Read the warning
   - Touch "Continue"

2. **Disk Selection**
   - Touch the disk where you want to install TouchOS
   - Touch "Continue"

3. **Partition Confirmation**
   - Review the partition layout
   - Touch "Install Now"

4. **Installation Progress**
   - Watch the progress bar
   - Takes 2-5 minutes

5. **Complete**
   - Touch "Restart Now"
   - **Remove USB drive** when prompted

6. **First Boot**
   - System boots from hard drive
   - TouchOS desktop loads
   - Enjoy!

## Touch Screen Support

### Automatic Detection

TouchOS automatically detects and configures:
- ✅ Acer T230H (pre-calibrated for 1920x1080)
- ✅ Generic USB HID touchscreens
- ✅ eGalax touch controllers
- ✅ 3M MicroTouch displays
- ✅ Most USB touch devices

### If Touch Doesn't Work

1. **Check USB connection** - Plug in before booting
2. **Use Safe Mode** - Boot option #2 (works without graphics)
3. **Use keyboard/mouse** - All touch interfaces work with keyboard too

## Testing in Virtual Machine First (Recommended)

Before installing on real hardware, test in a VM:

```bash
# Test in QEMU
qemu-system-x86_64 -cdrom touchos-installer.iso -m 2G -enable-kvm

# Test in VirtualBox
# 1. Create new VM (Linux, 64-bit)
# 2. Attach touchos-installer.iso as CD
# 3. Boot VM
```

## Troubleshooting

### USB Won't Boot

**Problem**: Computer doesn't boot from USB

**Try**:
- Use a different USB port (USB 2.0 ports work better)
- Disable Secure Boot in BIOS/UEFI
- Enable USB Boot in BIOS
- Recreate USB with Rufus (try MBR instead of GPT)

### Black Screen After Boot

**Problem**: Nothing appears after selecting boot option

**Try**:
- Select "Install TouchOS (Safe Mode)" from boot menu
- Wait 30 seconds (hardware detection takes time)
- Check if monitor is on correct input

### No Touch Input Detected

**Problem**: Touch screen not working

**Try**:
- Plug in USB touchscreen BEFORE booting
- Use keyboard/mouse (installer works with both)
- Boot in Safe Mode
- Check if touchscreen shows up: `lsusb | grep -i touch`

### Installer Can't Find Disks

**Problem**: "No disks found" error

**Try**:
- Go to BIOS and switch SATA mode (AHCI vs IDE mode)
- Check if disk is connected properly
- Try different SATA port

## System Requirements

### Minimum

- **CPU**: 64-bit x86_64 (Intel Core 2 Duo, AMD Athlon 64 or newer)
- **RAM**: 512 MB
- **Storage**: 5 GB free space
- **Graphics**: Any VGA-compatible

### Recommended

- **CPU**: Intel Core i5/i7 or AMD Ryzen
- **RAM**: 2 GB or more
- **Storage**: 20 GB SSD
- **Graphics**: 1920x1080 display
- **Touch**: USB HID touchscreen

## What's Included in the ISO

- ✅ 64-bit TouchOS kernel with hardware support
- ✅ Live init system with automatic hardware detection
- ✅ USB driver support (USB 1.1, 2.0, 3.0)
- ✅ Touch screen drivers (USB HID, multitouch)
- ✅ Framebuffer graphics support
- ✅ Touch-based graphical installer
- ✅ Automatic partition manager
- ✅ GRUB2 bootloader (UEFI + BIOS)
- ✅ Hybrid ISO (works on CD and USB)

## Dual Boot Support

You can install TouchOS alongside Windows or Linux:

1. Windows will be automatically detected
2. GRUB boot menu will show both operating systems
3. Choose which OS to boot at startup

## After Installation

Once installed, TouchOS will:
- Boot automatically from hard drive
- Load touch desktop interface
- Provide access to all touch apps
- Work with keyboard/mouse if no touch screen

## Need More Help?

See detailed guides:
- `REAL_HARDWARE_GUIDE.md` - Complete hardware deployment guide
- `INSTALLER_README.md` - Installer system documentation
- `ISO_BUILD_GUIDE.md` - How to rebuild the ISO

## Quick Reference Card

```
┌─────────────────────────────────────────────────────────────┐
│ TouchOS USB Boot - Quick Reference                          │
├─────────────────────────────────────────────────────────────┤
│                                                              │
│ 1. Write ISO to USB:                                         │
│    sudo dd if=touchos-installer.iso of=/dev/sdX bs=4M      │
│                                                              │
│ 2. Boot from USB:                                            │
│    Insert USB → Restart → Press F12 → Select USB            │
│                                                              │
│ 3. Install:                                                  │
│    Select "Install TouchOS" → Follow touch installer        │
│                                                              │
│ 4. First boot:                                               │
│    Remove USB → Reboot → Enjoy TouchOS!                     │
│                                                              │
│ Boot menu keys: F12 (most), F11 (MSI), F9 (HP), ESC (HP)   │
│ BIOS keys: F2, DEL, F10                                      │
│                                                              │
│ Safe Mode: Use "Install TouchOS (Safe Mode)" if graphics    │
│            don't work                                        │
│                                                              │
└─────────────────────────────────────────────────────────────┘
```

---

**Ready?** Create your USB drive and boot it! The installer will handle the rest.

**ISO**: `touchos-installer.iso` (22MB)
**Checksum**: `touchos-installer.iso.md5`
**Architecture**: x86_64 (64-bit)
**Boot**: UEFI & Legacy BIOS
**Media**: USB, CD, DVD
