# TouchOS Installer ISO Build Guide

## Overview

This directory contains everything needed to build a bootable TouchOS installation ISO, similar to Ubuntu's installer ISO.

## Features

The TouchOS Installer ISO includes:

- **Touch-Optimized GUI Installer** - Beautiful, easy-to-use installer with touch interface
- **Disk Partitioning** - Automatic GPT partition table creation
- **UEFI & BIOS Support** - Boots on both modern UEFI and legacy BIOS systems
- **Live Environment** - Option to try TouchOS before installing
- **Safe Mode** - Fallback installer without graphics
- **Bootloader Installation** - Automatic GRUB2 configuration

## Quick Start

### Prerequisites

Install required packages:

```bash
# On Ubuntu/Debian:
sudo apt-get install grub-pc-bin grub-efi-amd64-bin xorriso gcc make

# On Fedora/RHEL:
sudo dnf install grub2-pc-modules grub2-efi-x64-modules xorriso gcc make

# On Arch:
sudo pacman -S grub xorriso gcc make
```

### Building the ISO

Simply run:

```bash
chmod +x build-iso.sh
./build-iso.sh
```

This will:
1. Check dependencies
2. Clean previous builds
3. Build the kernel
4. Build userland applications
5. Create ISO directory structure
6. Copy all files
7. Configure bootloader
8. Generate bootable ISO

Output: `touchos-installer.iso`

## Testing the ISO

### Using QEMU (Recommended)

```bash
# Basic test
qemu-system-x86_64 -cdrom touchos-installer.iso -m 2G

# With KVM acceleration (faster)
qemu-system-x86_64 -cdrom touchos-installer.iso -m 2G -enable-kvm

# UEFI mode test
qemu-system-x86_64 -cdrom touchos-installer.iso -m 2G -bios /usr/share/ovmf/OVMF.fd
```

### Using VirtualBox

1. Create a new VM
2. Select "Other/Unknown 64-bit"
3. Allocate at least 2GB RAM
4. Attach `touchos-installer.iso` as CD/DVD
5. Boot the VM

### Using VMware

1. Create a new VM
2. Select "I will install the operating system later"
3. Choose "Other Linux 5.x kernel 64-bit"
4. Edit VM settings and attach ISO to CD/DVD drive
5. Power on

## Creating Installation Media

### USB Drive (Linux)

```bash
# WARNING: This will erase all data on the USB drive!
# Replace /dev/sdX with your USB device (use lsblk to find it)

sudo dd if=touchos-installer.iso of=/dev/sdX bs=4M status=progress oflag=sync
```

### USB Drive (Windows)

Use [Rufus](https://rufus.ie/):
1. Download and run Rufus
2. Select your USB drive
3. Choose `touchos-installer.iso`
4. Click "Start"

### USB Drive (macOS)

```bash
# Find your USB device
diskutil list

# Unmount it (replace diskN with your device)
diskutil unmountDisk /dev/diskN

# Write ISO
sudo dd if=touchos-installer.iso of=/dev/rdiskN bs=4m

# Eject
diskutil eject /dev/diskN
```

### CD/DVD (Linux)

```bash
# Using wodim
wodim -v -dao dev=/dev/sr0 touchos-installer.iso

# Using brasero (GUI)
brasero touchos-installer.iso
```

## Installer Screens

The installer provides a modern, touch-friendly experience:

1. **Welcome Screen** - Introduction and warning about data loss
2. **Disk Selection** - Choose installation target disk
3. **Partition Confirmation** - Review partition layout before installation
4. **Installation Progress** - Real-time progress with status messages
5. **Completion Screen** - Success message with reboot option

## Partition Layout

The installer creates the following partition layout:

| Partition | Type | Size | Mount Point | Purpose |
|-----------|------|------|-------------|---------|
| 1 | FAT32 | 512 MB | /boot/efi | EFI System Partition |
| 2 | ext4 | Remaining | / | Root filesystem |

## Boot Options

The ISO provides several boot options:

- **Install TouchOS** - Launch the graphical installer
- **Install TouchOS (Safe Mode)** - Text-mode installer without graphics
- **Try TouchOS (Live Mode)** - Run TouchOS without installing
- **Boot from First Hard Drive** - Boot existing OS
- **Reboot** - Restart computer
- **Shutdown** - Power off computer

## Customization

### Modifying Boot Menu

Edit `iso-build/boot/grub/grub.cfg` before building the ISO.

### Adding Software

Place additional packages in `iso-build/touchos/packages/` before building.

### Changing Installer Behavior

Edit `userland/touch-apps/installer.c` and rebuild.

## Troubleshooting

### ISO Build Fails

**Problem:** Missing dependencies

```
ERROR: Missing required dependencies: grub-mkrescue xorriso
```

**Solution:** Install missing packages (see Prerequisites)

### ISO Won't Boot

**Problem:** UEFI/BIOS compatibility

**Solution:** Try different boot mode in VM/BIOS settings

### Installer Crashes

**Problem:** Graphics driver issues

**Solution:** Use "Install TouchOS (Safe Mode)" option

### Disk Not Detected

**Problem:** Installer can't find disks

**Solution:**
1. Ensure you're running as root
2. Check if disk is visible in `/dev/`
3. Update `installer_disks.txt` manually

## File Structure

```
TouchOS-build/
├── build-iso.sh              # ISO build script
├── iso-build/                # ISO staging directory
│   ├── boot/
│   │   ├── grub/
│   │   │   └── grub.cfg      # Bootloader config
│   │   └── kernel.elf        # TouchOS kernel
│   └── touchos/
│       ├── bin/              # Userland applications
│       ├── lib/              # Libraries
│       └── kernel/           # Kernel backup
├── kernel.elf                # Built kernel
├── userland/                 # Source code
│   ├── touch-apps/
│   │   └── installer.c       # Installer source
│   └── libtouch/             # Touch framework
└── touchos-installer.iso     # Final ISO file
```

## Advanced Usage

### Building with Custom Kernel

```bash
# Build your kernel first
make clean
make

# Then build ISO
./build-iso.sh
```

### Creating Network Install ISO

Modify the installer to download packages from network instead of bundling them.

### Multi-Language Support

Add language selection to installer and localize strings.

## ISO Verification

Always verify the ISO checksum after download:

```bash
md5sum -c touchos-installer.iso.md5
```

## License

TouchOS Installer - Created by floof<3

This installer is part of the TouchOS project.

## Support

For issues, questions, or contributions:
- Check the main README.md
- Review TOUCH_SYSTEM_COMPLETE.md
- Examine installer.c source code

---

**Note:** The ISO is a hybrid image that works on both CD/DVD and USB drives.
