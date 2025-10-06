# Building TouchOS

← [Back to README](README.md) | [API](API.md) | [Next: Development](DEVELOPMENT.md) →

## Prerequisites

### Required Packages

**Ubuntu/Debian**:
```bash
sudo apt-get install build-essential nasm grub-pc-bin grub-efi-amd64-bin \
    xorriso mtools gcc make qemu-system-x86
```

**Fedora/RHEL**:
```bash
sudo dnf install gcc make nasm grub2-tools xorriso mtools qemu-system-x86
```

**Arch Linux**:
```bash
sudo pacman -S base-devel nasm grub xorriso mtools qemu
```

## Building

### 1. Build Kernel

```bash
make clean
make
```

**Output**: `kernel.elf` (64-bit ELF executable)

### 2. Build Userland

```bash
cd userland
make
```

**Output**: `libtouch.a`, touch apps, package manager

### 3. Build ISO

```bash
./build-iso.sh
```

**Output**: `touchos-installer.iso` (22 MB bootable ISO)

## Testing

### QEMU

```bash
# Test kernel
qemu-system-x86_64 -kernel kernel.elf -serial stdio

# Test ISO
qemu-system-x86_64 -cdrom touchos-installer.iso -m 2G -enable-kvm
```

### VirtualBox

1. Create new VM (64-bit Linux)
2. Allocate 2 GB RAM
3. Attach `touchos-installer.iso`
4. Boot

## Clean Build

```bash
make clean
cd userland && make clean
rm -rf iso-build touchos-installer.iso
```

← [Back to README](README.md) | [API](API.md) | [Next: Development](DEVELOPMENT.md) →
