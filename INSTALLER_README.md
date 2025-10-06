# TouchOS Installer System

## Overview

TouchOS now includes a complete installer system with a modern touch-friendly graphical interface.

## What's Included

### 1. Touch-Based GUI Installer (`userland/touch-apps/installer.c`)

A beautiful, full-featured installer with:

- **Welcome Screen** - Introduction with warning about data loss
- **Disk Selection** - Visual disk chooser with device information
- **Partition Preview** - Shows planned partition layout before installation
- **Installation Progress** - Real-time progress bar with status updates
- **Completion Screen** - Success confirmation with reboot option
- **Error Handling** - Graceful error recovery with retry option

### 2. Automated Installation Features

- **GPT Partition Table** - Modern partitioning with UEFI support
- **Dual Boot Support** - EFI System Partition (512MB) + Root partition
- **File System Creation** - Automatic FAT32 and ext4 formatting
- **Bootloader Installation** - GRUB2 configuration for UEFI and BIOS
- **System File Deployment** - Kernel and userland application installation

### 3. Bootable ISO (`touchos-installer.iso`)

A 22MB bootable ISO image that includes:

- TouchOS kernel
- Touch-based installer
- Userland applications
- Libraries
- GRUB2 bootloader (UEFI & BIOS)

## Quick Start

### Building the ISO

```bash
./build-iso.sh
```

### Testing in QEMU

```bash
qemu-system-x86_64 -cdrom touchos-installer.iso -m 2G -enable-kvm
```

### Creating USB Installation Media

```bash
sudo dd if=touchos-installer.iso of=/dev/sdX bs=4M status=progress
```

Replace `/dev/sdX` with your USB device.

## Boot Menu Options

When you boot the ISO, you'll see:

1. **Install TouchOS** - Launch graphical installer
2. **Install TouchOS (Safe Mode)** - Text-mode installer without graphics
3. **Try TouchOS (Live Mode)** - Run TouchOS without installing
4. **Boot from First Hard Drive** - Boot existing OS
5. **Reboot** - Restart computer
6. **Shutdown** - Power off computer

## Installation Process

### Step 1: Welcome

The installer greets you with a warning about data loss and information about the installation process.

### Step 2: Disk Selection

Choose which disk to install TouchOS on. The installer shows:
- Device name (e.g., `/dev/sda`)
- Disk size
- Model name

### Step 3: Partition Confirmation

Review the partition layout:
- **EFI System Partition**: 512 MB (FAT32)
- **TouchOS Root**: Remaining space (ext4)

### Step 4: Installation

Watch the progress bar as the installer:
1. Creates partitions
2. Formats filesystems
3. Mounts partitions
4. Installs system files
5. Installs bootloader

### Step 5: Complete

Remove installation media and reboot into your new TouchOS installation!

## Technical Details

### Partition Layout

| Partition | Type | Size | FS Type | Mount | Purpose |
|-----------|------|------|---------|-------|---------|
| /dev/sdX1 | EFI System | 512 MB | FAT32 | /boot/efi | Boot files |
| /dev/sdX2 | Linux | Remaining | ext4 | / | Root filesystem |

### Directory Structure

The installer creates the following structure:

```
/mnt/touchos/
├── boot/
│   ├── efi/              # EFI boot files
│   ├── grub/
│   │   └── grub.cfg      # Boot configuration
│   └── kernel.elf        # TouchOS kernel
├── bin/                  # Userland applications
├── lib/                  # Libraries
├── etc/                  # Configuration files
├── home/                 # User directories
├── var/                  # Variable data
└── tmp/                  # Temporary files
```

### Installer Architecture

The installer uses a state machine with these states:

1. `INSTALLER_WELCOME` - Initial screen
2. `INSTALLER_DISK_SELECT` - Disk chooser
3. `INSTALLER_PARTITION_CONFIRM` - Confirmation screen
4. `INSTALLER_INSTALLING` - Installation in progress
5. `INSTALLER_COMPLETE` - Success screen
6. `INSTALLER_ERROR` - Error handling

## Files

### Build System

- `build-iso.sh` - ISO builder script
- `ISO_BUILD_GUIDE.md` - Detailed ISO building documentation

### Installer

- `userland/touch-apps/installer.c` - Main installer source code
- `installer_disks.txt` - Disk configuration (optional)

### Output

- `touchos-installer.iso` - Bootable ISO image (22MB)
- `touchos-installer.iso.md5` - Checksum verification
- `iso-build/` - ISO staging directory

## Customization

### Change Partition Sizes

Edit `installer.c` at line 112-123:

```c
// Create EFI partition (512MB)
snprintf(cmd, sizeof(cmd),
         "parted -s %s mkpart ESP fat32 1MiB 513MiB", device);
```

### Modify Boot Menu

Edit `iso-build/boot/grub/grub.cfg` after building.

### Add Pre-loaded Disks

Edit `installer_disks.txt`:

```
/dev/sda,500GB,Samsung SSD 860 EVO
/dev/nvme0n1,1TB,WD Black NVMe
```

## Safety Features

- **Root Check** - Installer requires root privileges
- **Confirmation Dialog** - User must confirm before erasing disk
- **Progress Tracking** - Real-time status updates
- **Error Recovery** - Graceful handling of installation failures
- **Mount Cleanup** - Automatic unmounting on error

## Compatibility

### UEFI Systems

- EFI partition for UEFI boot
- GRUB2 EFI bootloader
- Secure Boot compatible (if configured)

### Legacy BIOS Systems

- MBR compatibility mode
- GRUB2 BIOS bootloader
- Hybrid boot support

### Tested On

- QEMU/KVM
- VirtualBox
- VMware
- Real hardware with UEFI
- Real hardware with Legacy BIOS

## Troubleshooting

### "Installer must be run as root"

Run the installer with sudo:
```bash
sudo touch-installer
```

### "Failed to partition disk"

- Check if disk is in use
- Verify disk path is correct
- Ensure no existing mounts

### ISO won't boot

- Check BIOS/UEFI boot settings
- Try different boot mode
- Verify ISO integrity with checksum

### Graphics don't work

Use Safe Mode boot option for text-based installation.

## Advanced Usage

### Network Installation

Modify `install_system()` to download packages from a repository instead of copying from ISO.

### Custom Disk Layout

Modify `partition_disk()` to create custom partition schemes (e.g., separate /home partition).

### Multi-Boot Configuration

The installer supports dual-boot. The boot menu includes "Boot from First Hard Drive" option.

## Performance

- **ISO Size**: 22 MB
- **Installation Time**: ~2-5 minutes (depending on disk speed)
- **Minimum RAM**: 512 MB
- **Recommended RAM**: 2 GB
- **Disk Space**: Minimum 1 GB, recommended 10 GB

## Next Steps

After installation:

1. Remove installation media
2. Reboot computer
3. TouchOS will boot automatically
4. Log in and enjoy your new touch-optimized OS!

## Credits

Created by: floof<3

Part of the TouchOS project - A touch-optimized operating system for modern hardware.

---

For more information:
- See `ISO_BUILD_GUIDE.md` for detailed build instructions
- See `TOUCH_SYSTEM_COMPLETE.md` for system overview
- See `installer.c` source code for implementation details
