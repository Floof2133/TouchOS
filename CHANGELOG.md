# TouchOS Changelog

← [Back to README](README.md) →

## Version 1.0.0 (2024-10-06)

### 🎉 Initial Release

**Core System**
- ✅ 64-bit x86_64 kernel
- ✅ Physical memory manager (PMM)
- ✅ Heap allocator (kmalloc/kfree)
- ✅ Serial debugging support
- ✅ Multiboot2 boot protocol

**Drivers**
- ✅ USB subsystem (XHCI, EHCI, UHCI)
- ✅ USB touchscreen driver (Acer T230H)
- ✅ Framebuffer graphics (1920×1080)
- ✅ Serial port driver (COM1/COM2)

**User Interface**
- ✅ libtouch touch framework
- ✅ Touch event handling (10-point multitouch)
- ✅ Gesture recognition (tap, swipe, pinch, rotate)
- ✅ UI widgets (buttons, lists, text inputs, sliders)
- ✅ On-screen keyboard
- ✅ Double-buffered rendering

**Applications**
- ✅ Touch Launcher (desktop)
- ✅ File Manager
- ✅ Settings
- ✅ Terminal
- ✅ System Monitor
- ✅ Touch Installer

**Package Manager**
- ✅ tpkg CLI tool
- ✅ tpkg-build (package builder)
- ✅ tpkg-touch-gui (touch interface)
- ✅ .tpkg package format

**Installer**
- ✅ Bootable ISO (22 MB)
- ✅ Touch-based installer
- ✅ Automatic partitioning (GPT)
- ✅ GRUB2 bootloader installation
- ✅ USB boot support
- ✅ Live environment with hardware detection

**Documentation**
- ✅ Comprehensive documentation suite
- ✅ Architecture guide
- ✅ Kernel documentation
- ✅ Driver documentation
- ✅ API reference
- ✅ Build guide
- ✅ Development guide
- ✅ Contributing guide
- ✅ FAQ
- ✅ Real hardware deployment guide

### Hardware Support

**Tested On**:
- Acer T230H (1920×1080 touchscreen)
- Dell Inspiron 13 7370 (Intel i5/i7-8th gen)
- QEMU/KVM
- VirtualBox

## Roadmap

### Version 1.1 (Planned)
- [ ] Network stack (TCP/IP)
- [ ] WiFi driver
- [ ] Multi-process scheduler
- [ ] Sound subsystem
- [ ] Additional touch device support

### Version 2.0 (Future)
- [ ] GPU acceleration
- [ ] Virtual file system (VFS)
- [ ] Process isolation/sandboxing
- [ ] Power management
- [ ] SMP (multi-core) support

---

← [Back to README](README.md) →
