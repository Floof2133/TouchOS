# TouchOS Boot Fix Guide

## Issues Fixed

### 1. ✅ GRUB Can't Find Kernel (FIXED)

**Problem**: GRUB config was using `linux` command for a Multiboot kernel
**Solution**: Changed to `multiboot` command

**Before**:
```
linux /boot/kernel.elf
```

**After**:
```
multiboot /boot/kernel.elf
```

### 2. ✅ ISO Corruption with Etcher (FIXED)

**Problem**: `isohybrid` was corrupting the ISO
**Solution**: Removed isohybrid from build process

### 3. ✅ Hybrid Boot Issues (FIXED)

**Problem**: ISO wasn't properly structured for USB boot
**Solution**: Let GRUB handle hybrid boot naturally

## Testing the Fixed ISO

### Test in QEMU (No KVM needed)

```bash
# Test without KVM (works on any system)
qemu-system-x86_64 -cdrom touchos-installer.iso -m 512

# With serial output for debugging
qemu-system-x86_64 -cdrom touchos-installer.iso -m 512 -serial stdio

# If you have KVM available
qemu-system-x86_64 -cdrom touchos-installer.iso -m 2G -enable-kvm
```

### Create USB Drive (Won't Crash Etcher)

The ISO is now a standard hybrid ISO. Use these methods:

#### Method 1: dd (Linux/macOS) - Most Reliable

```bash
# Find your USB device
lsblk  # or: sudo fdisk -l

# Unmount if mounted
sudo umount /dev/sdX*

# Write ISO (this will work now)
sudo dd if=touchos-installer.iso of=/dev/sdX bs=4M status=progress oflag=sync conv=fdatasync

# Verify
sudo sync
```

#### Method 2: Etcher (Should work now)

The ISO should no longer crash Etcher. If it still does:

1. Use Rufus (Windows) instead
2. Select DD mode (not ISO mode)
3. Write and verify

#### Method 3: Rufus (Windows)

1. Open Rufus
2. Select USB drive
3. Select `touchos-installer.iso`
4. **Important**: Choose "DD Image" mode if prompted
5. Click Start

#### Method 4: Ventoy

Ventoy should work perfectly:
```bash
# Copy ISO to Ventoy USB
cp touchos-installer.iso /path/to/ventoy/
```

## Real Hardware Boot

### BIOS/UEFI Settings

**Before booting**:

1. **Disable Secure Boot** (UEFI Settings)
   - TouchOS doesn't have signed bootloader
   - Must be disabled

2. **Enable USB Boot** (Boot Settings)
   - Move USB to top of boot order
   - Or use F12/F11 boot menu

3. **Check SATA Mode** (if installing)
   - Set to AHCI (not RAID or IDE)
   - Required for disk detection

### Boot Process

1. **Insert USB drive**
2. **Restart computer**
3. **Press boot menu key** (F12, F11, ESC, or F8)
4. **Select USB drive**
5. **GRUB menu should appear**
6. **Select "Install TouchOS"**

### If GRUB Doesn't Appear

**Symptom**: Black screen or "No bootable device"

**Fixes**:
1. Recreate USB with `dd` method
2. Disable Secure Boot in UEFI
3. Try different USB port (USB 2.0 often more reliable)
4. Check BIOS boot mode (try Legacy if UEFI fails, or vice versa)

### If GRUB Appears But Kernel Doesn't Load

**Symptom**: GRUB menu works, but selecting TouchOS fails

**Check**:
1. Kernel is in ISO: `xorriso -indev touchos-installer.iso -find | grep kernel`
2. GRUB config is correct (should use `multiboot`)
3. Rebuild ISO: `./build-iso.sh`

## Debugging Boot Issues

### Check ISO Contents

```bash
# List all files in ISO
xorriso -indev touchos-installer.iso -find

# Check if kernel exists
xorriso -indev touchos-installer.iso -find | grep kernel.elf

# Should show: /boot/kernel.elf
```

### Check Kernel Multiboot Header

```bash
# Verify kernel has Multiboot header
xxd kernel.elf | head -20 | grep "1badb002"

# Should find: 0x1BADB002 (Multiboot magic)
```

### Serial Debug Output

Boot with serial console to see kernel output:

```bash
qemu-system-x86_64 -cdrom touchos-installer.iso -m 512 -serial stdio
```

## USB Creation Comparison

| Method | Reliability | Speed | Notes |
|--------|-------------|-------|-------|
| dd | ⭐⭐⭐⭐⭐ | Fast | Most reliable, direct write |
| Rufus (DD mode) | ⭐⭐⭐⭐⭐ | Fast | Works great on Windows |
| Etcher | ⭐⭐⭐⭐ | Medium | Should work now |
| Rufus (ISO mode) | ⭐⭐⭐ | Fast | May have issues |
| Ventoy | ⭐⭐⭐⭐⭐ | Instant | Just copy ISO |

## Expected Boot Sequence

```
1. BIOS/UEFI loads GRUB from USB
   ↓
2. GRUB displays boot menu
   ↓
3. User selects "Install TouchOS"
   ↓
4. GRUB loads kernel using multiboot
   ↓
5. Kernel transitions to 64-bit mode
   ↓
6. Kernel initializes (serial output visible)
   ↓
7. System ready (or kernel panic if issue)
```

## Common Error Messages

### "Error: file '/boot/kernel.elf' not found"

**Cause**: Kernel not in ISO or wrong path
**Fix**: Rebuild ISO with `./build-iso.sh`

### "Error: invalid magic number"

**Cause**: Kernel not Multiboot-compliant
**Fix**: Check kernel has Multiboot header

### "Error: no multiboot header found"

**Cause**: GRUB can't find Multiboot header in kernel
**Fix**: Verify kernel compilation, check boot64.asm

### Black screen after GRUB

**Cause**: Kernel crashed during boot
**Fix**: Test with serial console to see error

## Verification Checklist

Before creating USB:

- [ ] ISO exists: `ls -lh touchos-installer.iso`
- [ ] ISO is valid: `file touchos-installer.iso`
- [ ] Checksum OK: `md5sum -c touchos-installer.iso.md5`
- [ ] Kernel in ISO: `xorriso -indev touchos-installer.iso -find | grep kernel`
- [ ] Size is ~22MB: `du -h touchos-installer.iso`

Before booting on hardware:

- [ ] Secure Boot disabled
- [ ] USB Boot enabled
- [ ] SATA in AHCI mode
- [ ] USB drive created with `dd` or Rufus DD mode

## Quick Test Commands

```bash
# Rebuild ISO
./build-iso.sh

# Verify ISO
file touchos-installer.iso
md5sum -c touchos-installer.iso.md5

# Test in QEMU
qemu-system-x86_64 -cdrom touchos-installer.iso -m 512

# Create USB (replace sdX)
sudo dd if=touchos-installer.iso of=/dev/sdX bs=4M status=progress oflag=sync
```

## Still Having Issues?

1. **Check kernel compiles**: `make clean && make`
2. **Verify Multiboot header**: `xxd kernel.elf | head -20`
3. **Test kernel directly**: `qemu-system-x86_64 -kernel kernel.elf`
4. **Check GRUB config**: `cat iso-build/boot/grub/grub.cfg`
5. **Rebuild from scratch**: `make clean && ./build-iso.sh`

---

**The ISO should now:**
- ✅ Boot in QEMU (with or without KVM)
- ✅ Boot from USB on real hardware
- ✅ Not crash Etcher
- ✅ Work with dd, Rufus, and Ventoy
