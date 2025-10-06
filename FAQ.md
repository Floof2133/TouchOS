# TouchOS Frequently Asked Questions

← [Back to README](README.md) →

## General Questions

**Q: What is TouchOS?**  
A: A custom 64-bit operating system built from scratch for touch-screen interfaces, specifically optimized for the Acer T230H touchscreen with embedded Dell Inspiron 13 7370 hardware.

**Q: Why create a new OS?**  
A: To learn OS development, create a fully customized system for specific hardware, and build a truly touch-first interface without X11/Wayland overhead.

**Q: Is it production-ready?**  
A: It's a learning project. Use for education, experimentation, or personal projects. Not recommended for critical systems.

## Installation

**Q: How do I install TouchOS?**  
A: Create a bootable USB from `touchos-installer.iso`, boot from it, and follow the touch installer. See [USB_BOOT_INSTRUCTIONS.md](USB_BOOT_INSTRUCTIONS.md).

**Q: Can I dual-boot with Windows/Linux?**  
A: Yes! The installer detects existing operating systems and adds them to the GRUB menu.

**Q: What hardware do I need?**  
A: Minimum: x86_64 CPU, 512 MB RAM, 5 GB storage. Recommended: Multi-core CPU, 2+ GB RAM, SSD, USB touchscreen.

## Hardware

**Q: Does my touchscreen work?**  
A: USB HID touchscreens are supported. Acer T230H is pre-configured. Others may need calibration.

**Q: Can I use keyboard/mouse?**  
A: Yes! All touch interfaces work with keyboard and mouse too.

**Q: Does it work on tablets?**  
A: Only x86_64 tablets. ARM tablets are not supported.

**Q: What about laptops?**  
A: Yes, if they're x86_64. Touch features work best with touchscreen.

## Development

**Q: How do I build TouchOS?**  
A: See [BUILDING.md](BUILDING.md) for complete build instructions.

**Q: Can I contribute?**  
A: Absolutely! See [CONTRIBUTING.md](CONTRIBUTING.md).

**Q: What language is it written in?**  
A: C for kernel/drivers/userland, NASM assembly for boot code.

**Q: How do I add my own app?**  
A: See [DEVELOPMENT.md](DEVELOPMENT.md) - create a C file, link with libtouch.a.

## Technical

**Q: What bootloader does it use?**  
A: GRUB2 with Multiboot2 protocol.

**Q: Does it support UEFI?**  
A: Yes! Both UEFI and Legacy BIOS are supported.

**Q: Is there networking?**  
A: Not yet - it's on the roadmap.

**Q: Can it run Linux programs?**  
A: No - it's a completely different OS. Programs must be built specifically for TouchOS.

## Troubleshooting

**Q: USB won't boot**  
A: Try different USB port, disable Secure Boot, enable USB Boot in BIOS.

**Q: Touch not working**  
A: Ensure USB touchscreen is connected before booting. Try Safe Mode if graphics fail.

**Q: Black screen after boot**  
A: Boot in Safe Mode, or add `nomodeset` kernel parameter.

**Q: Installer can't find disks**  
A: Change SATA mode in BIOS (AHCI vs IDE).

**Q: Where are the logs?**  
A: Serial output (COM1) - use `qemu-system-x86_64 -serial stdio` to view.

← [Back to README](README.md) →
