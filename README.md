# 🖐️ TouchOS

> **A Touch-First Operating System Built from Scratch**

TouchOS is a custom 64-bit x86_64 operating system designed specifically for touchscreen interfaces. Everything from the kernel to the graphical touch interface is written from scratch in C and Assembly.

[![License: MIT](https://img.shields.io/badge/License-MIT-blue.svg)](LICENSE.md)
[![Platform: x86_64](https://img.shields.io/badge/Platform-x86__64-green.svg)](ARCHITECTURE.md)
[![Build](https://img.shields.io/badge/Build-Passing-success.svg)](BUILDING.md)

---

## 🎯 Features

### Core System
- ✅ **64-bit Kernel** - Custom x86_64 kernel with memory management
- ✅ **USB Support** - Full USB stack (1.1, 2.0, 3.0)
- ✅ **Touch Input** - Native 10-point multitouch support
- ✅ **Graphics** - Framebuffer graphics (1920×1080)
- ✅ **Package Manager** - Custom `.tpkg` package system with GUI

### Touch Interface
- ✅ **libtouch Framework** - Complete touch UI API
- ✅ **Gesture Recognition** - Tap, swipe, pinch, rotate
- ✅ **Touch Apps** - File manager, settings, terminal, and more
- ✅ **On-Screen Keyboard** - QWERTY, numeric, and symbol layouts
- ✅ **44px Touch Targets** - Ergonomically optimized for fingers

### Installation
- ✅ **Bootable ISO** - 22 MB installer (USB/CD/DVD)
- ✅ **Touch Installer** - Ubuntu-style graphical installer
- ✅ **Auto Partitioning** - GPT with EFI + Root partitions
- ✅ **UEFI & BIOS** - Supports both boot modes
- ✅ **Dual Boot** - Coexists with Windows/Linux

---

## 🚀 Quick Start

### Download & Install

**1. Build the ISO**
```bash
./build-iso.sh
```
Output: `touchos-installer.iso` (22 MB)

**2. Create USB Installer**
```bash
sudo dd if=touchos-installer.iso of=/dev/sdX bs=4M status=progress
```

**3. Boot & Install**
- Boot from USB
- Select "Install TouchOS"
- Follow touch installer
- Reboot and enjoy!

**See [USB_BOOT_INSTRUCTIONS.md](USB_BOOT_INSTRUCTIONS.md) for detailed instructions.**

### Testing in QEMU

```bash
qemu-system-x86_64 -cdrom touchos-installer.iso -m 2G -enable-kvm
```

---

## 📖 Documentation

### Getting Started
- **[USB Boot Instructions](USB_BOOT_INSTRUCTIONS.md)** - Create bootable USB and install
- **[Hardware Guide](REAL_HARDWARE_GUIDE.md)** - Real hardware deployment
- **[FAQ](FAQ.md)** - Frequently asked questions

### System Documentation
- **[Architecture](ARCHITECTURE.md)** - System architecture overview
- **[Kernel](KERNEL.md)** - Kernel internals and memory management
- **[Drivers](DRIVERS.md)** - Device driver documentation
- **[Touch System](TOUCH_SYSTEM_COMPLETE.md)** - Complete touch system guide
- **[Touch Interface](TOUCH_INTERFACE_GUIDE.md)** - UI/UX design guide

### Developer Documentation
- **[API Reference](API.md)** - libtouch framework API
- **[Building](BUILDING.md)** - Build instructions
- **[Development Guide](DEVELOPMENT.md)** - Development workflow
- **[Contributing](CONTRIBUTING.md)** - How to contribute
- **[Touch Apps](TOUCH_APPS_COMPLETE.md)** - Application development

### Package Manager
- **[Package Manager](PACKAGE_MANAGER.md)** - tpkg documentation
- **[Package System](PACKAGE_SYSTEM_SUMMARY.md)** - Package format and repository
- **[Quick Start](QUICKSTART_PACKAGE_MANAGER.md)** - Package manager quick guide

### Advanced
- **[ISO Building](ISO_BUILD_GUIDE.md)** - Build custom ISOs
- **[Installer](INSTALLER_README.md)** - Installer system documentation
- **[Hardware Setup](HARDWARE_SETUP.md)** - Hardware configuration

### Project
- **[Changelog](CHANGELOG.md)** - Version history and roadmap
- **[License](LICENSE.md)** - MIT License

---

## 💻 System Requirements

### Minimum
- **CPU**: x86_64 (64-bit Intel/AMD)
- **RAM**: 512 MB
- **Storage**: 5 GB
- **Graphics**: VGA-compatible

### Recommended
- **CPU**: Intel Core i5/i7 or AMD Ryzen
- **RAM**: 2 GB+
- **Storage**: 20 GB SSD
- **Display**: 1920×1080 touchscreen
- **Touch**: USB HID multitouch device

### Tested Hardware
- ✅ **Acer T230H** (1920×1080, 10-point USB touch) - Fully supported
- ✅ **Dell Inspiron 13 7370** (Intel i5-8250U/i7-8550U) - Primary target
- ✅ **QEMU/KVM** - Virtualization testing
- ✅ **VirtualBox** - VM testing

---

## 🏗️ Architecture

```
┌─────────────────────────────────────────────────┐
│           User Applications                      │
│  Launcher • File Manager • Terminal • Settings  │
└─────────────────────────────────────────────────┘
                      ↕
┌─────────────────────────────────────────────────┐
│         libtouch Framework                       │
│  Touch Events • Widgets • Drawing • Gestures    │
└─────────────────────────────────────────────────┘
                      ↕
┌─────────────────────────────────────────────────┐
│          TouchOS Kernel (64-bit)                 │
│    PMM • Heap • Scheduler • System Calls        │
└─────────────────────────────────────────────────┘
                      ↕
┌─────────────────────────────────────────────────┐
│            Device Drivers                        │
│  USB Stack • Touch Input • Framebuffer          │
└─────────────────────────────────────────────────┘
                      ↕
┌─────────────────────────────────────────────────┐
│              Hardware                            │
│   CPU • RAM • USB • Display • Touch Screen      │
└─────────────────────────────────────────────────┘
```

**See [ARCHITECTURE.md](ARCHITECTURE.md) for detailed architecture documentation.**

---

## 🛠️ Building from Source

### Prerequisites
```bash
# Ubuntu/Debian
sudo apt-get install build-essential nasm grub-pc-bin grub-efi-amd64-bin xorriso mtools qemu-system-x86

# Fedora
sudo dnf install gcc make nasm grub2-tools xorriso mtools qemu-system-x86

# Arch
sudo pacman -S base-devel nasm grub xorriso mtools qemu
```

### Build
```bash
# Build kernel
make

# Build ISO
./build-iso.sh
```

**See [BUILDING.md](BUILDING.md) for complete build guide.**

---

## 📦 Package Manager

TouchOS includes **tpkg** - a custom package manager with three components:

1. **tpkg** (CLI) - Install/remove/update packages
2. **tpkg-build** - Create .tpkg packages
3. **tpkg-touch-gui** - Touch interface for package management

```bash
# Install package
tpkg install myapp

# Search packages
tpkg search game

# Update all
tpkg update
```

**See [PACKAGE_MANAGER.md](PACKAGE_MANAGER.md) for details.**

---

## 🎨 Applications

### Desktop & Core
- **Touch Launcher** - App grid desktop
- **File Manager** - Browse and manage files
- **Settings** - System configuration
- **Terminal** - Command-line interface
- **System Monitor** - Resource monitoring

### System
- **Touch Installer** - OS installation wizard
- **Package Manager GUI** - Visual package management

**See [TOUCH_APPS_COMPLETE.md](TOUCH_APPS_COMPLETE.md) for application details.**

---

## 🤝 Contributing

We welcome contributions! Here's how you can help:

- 🐛 **Report Bugs** - Open an issue with details
- 💡 **Suggest Features** - Share your ideas
- 🔧 **Submit Code** - Create a pull request
- 📚 **Improve Docs** - Help with documentation
- 🧪 **Test** - Try on different hardware

**See [CONTRIBUTING.md](CONTRIBUTING.md) for contribution guidelines.**

---

## 🎯 Project Goals

### Why TouchOS?

1. **Learn OS Development** - Understand operating systems from the ground up
2. **Touch-First Design** - Build UI truly optimized for touch (not retrofitted)
3. **Hardware Optimization** - Maximize performance for specific hardware
4. **Full Control** - Complete customization without legacy constraints
5. **Educational** - Teach others about OS internals

### Design Philosophy

- **Touch-First**: Every UI element designed for finger input
- **Simplicity**: Clean, minimal, focused design
- **Performance**: Direct hardware access, no bloat
- **Custom**: Built for Acer T230H + Dell Inspiron 13 7370

---

## 📊 Project Stats

- **Language**: C (kernel, drivers, userland) + NASM (boot code)
- **Lines of Code**: ~5,000+
- **Kernel Size**: ~50 KB
- **ISO Size**: 22 MB
- **Boot Time**: ~5-10 seconds
- **Touch Latency**: <10ms

---

## 🗺️ Roadmap

### Version 1.0 ✅
- [x] 64-bit kernel with memory management
- [x] USB drivers (1.1, 2.0, 3.0)
- [x] Touch input driver (Acer T230H)
- [x] Framebuffer graphics
- [x] libtouch framework
- [x] Touch applications suite
- [x] Package manager (tpkg)
- [x] Bootable ISO installer

### Version 1.1 (Planned)
- [ ] Network stack (TCP/IP)
- [ ] WiFi driver
- [ ] Multi-process scheduler
- [ ] Sound subsystem
- [ ] Additional touch device support

### Version 2.0 (Future)
- [ ] GPU acceleration
- [ ] Virtual file system (VFS)
- [ ] Process isolation
- [ ] Power management
- [ ] SMP (multi-core) support

**See [CHANGELOG.md](CHANGELOG.md) for version history.**

---

## ⚠️ Disclaimer

TouchOS is an educational and experimental project. It is:

- ✅ Great for learning OS development
- ✅ Suitable for experimentation and personal use
- ✅ A platform for testing touch interfaces
- ❌ **Not production-ready**
- ❌ **Not recommended for critical systems**
- ❌ **Use at your own risk**

The authors are not responsible for any hardware damage, data loss, or other issues.

---

## 📜 License

TouchOS is released under the **MIT License**. See [LICENSE.md](LICENSE.md) for details.

### Credits

- **Created by**: floof<3
- **Special Thanks**: [XansiVA](https://github.com/XansiVA) for project guidance
- **Target Hardware**: Acer T230H, Dell Inspiron 13 7370

---

## 💬 Community

- **Issues**: [GitHub Issues](https://github.com/yourusername/TouchOS/issues)
- **Discussions**: [GitHub Discussions](https://github.com/yourusername/TouchOS/discussions)
- **Pull Requests**: Welcome!

---

## 📸 Screenshots

```
┌────────────────────────────────────────┐
│  🖐️ TouchOS Desktop                   │
│                                         │
│  ┌─────┐ ┌─────┐ ┌─────┐ ┌─────┐     │
│  │ 📁  │ │ ⚙️  │ │ 💻  │ │ 📦  │     │
│  │File │ │Set- │ │Term-│ │ Pkg │     │
│  │ Mgr │ │tings│ │inal │ │ Mgr │     │
│  └─────┘ └─────┘ └─────┘ └─────┘     │
│                                         │
│  Large touch-friendly buttons          │
│  10-point multitouch support           │
│  Gesture recognition                   │
│  On-screen keyboard                    │
└────────────────────────────────────────┘
```

---

## 🔗 Quick Links

| Category | Links |
|----------|-------|
| **Get Started** | [USB Boot](USB_BOOT_INSTRUCTIONS.md) • [Install](REAL_HARDWARE_GUIDE.md) • [FAQ](FAQ.md) |
| **Documentation** | [Architecture](ARCHITECTURE.md) • [Kernel](KERNEL.md) • [Drivers](DRIVERS.md) • [API](API.md) |
| **Development** | [Building](BUILDING.md) • [Contributing](CONTRIBUTING.md) • [Development](DEVELOPMENT.md) |
| **Package Manager** | [tpkg Docs](PACKAGE_MANAGER.md) • [Quick Start](QUICKSTART_PACKAGE_MANAGER.md) |
| **Project** | [Changelog](CHANGELOG.md) • [License](LICENSE.md) |

---

<p align="center">
  <strong>TouchOS - Making Touch Interfaces Native, One Touch at a Time</strong><br>
  Built with ❤️ (and lots of coffee ☕) by floof<3
</p>

<p align="center">
  <sub>Version 1.0.0 | 2024 | MIT License</sub>
</p>
