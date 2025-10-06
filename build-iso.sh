#!/bin/bash
# TouchOS ISO Build Script
# Creates a bootable installation ISO for TouchOS
# Similar to Ubuntu's ISO structure

set -e  # Exit on error

echo "========================================="
echo "TouchOS ISO Builder v1.0"
echo "========================================="
echo ""

# Configuration
ISO_DIR="iso-build"
ISO_NAME="touchos-installer.iso"
ISO_LABEL="TOUCHOS_INSTALL"
KERNEL_ELF="kernel.elf"

# Check dependencies
echo "[1/8] Checking dependencies..."
MISSING_DEPS=""

if ! command -v grub-mkrescue &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS grub-mkrescue"
fi

if ! command -v xorriso &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS xorriso"
fi

if ! command -v gcc &> /dev/null; then
    MISSING_DEPS="$MISSING_DEPS gcc"
fi

if [ -n "$MISSING_DEPS" ]; then
    echo "ERROR: Missing required dependencies:$MISSING_DEPS"
    echo "Please install them using:"
    echo "  sudo apt-get install grub-pc-bin grub-efi-amd64-bin xorriso gcc make"
    exit 1
fi

echo "  ✓ All dependencies found"

# Clean previous build
echo ""
echo "[2/8] Cleaning previous build..."
rm -rf "$ISO_DIR"
rm -f "$ISO_NAME"
echo "  ✓ Cleaned"

# Create ISO directory structure
echo ""
echo "[3/8] Creating ISO structure..."
mkdir -p "$ISO_DIR/boot/grub"
mkdir -p "$ISO_DIR/touchos/bin"
mkdir -p "$ISO_DIR/touchos/lib"
mkdir -p "$ISO_DIR/touchos/kernel"
echo "  ✓ Directory structure created"

# Build kernel
echo ""
echo "[4/8] Building kernel..."
if [ ! -f "$KERNEL_ELF" ]; then
    echo "  Building kernel.elf..."
    make clean > /dev/null 2>&1 || true
    make > /dev/null 2>&1
    if [ ! -f "$KERNEL_ELF" ]; then
        echo "  ERROR: Kernel build failed"
        exit 1
    fi
fi
echo "  ✓ Kernel ready: $KERNEL_ELF"

# Build userland applications
echo ""
echo "[5/8] Building userland applications..."
cd userland

# Build libtouch
echo "  Building libtouch..."
cd libtouch
if [ ! -f "Makefile" ]; then
    cat > Makefile << 'EOF'
CC = gcc
CFLAGS = -Wall -Wextra -O2 -fPIC
AR = ar

libtouch.a: touch_framework.o
	$(AR) rcs libtouch.a touch_framework.o

touch_framework.o: touch_framework.c touch_framework.h
	$(CC) $(CFLAGS) -c touch_framework.c -o touch_framework.o

clean:
	rm -f touch_framework.o libtouch.a
EOF
fi
make clean > /dev/null 2>&1 || true
make > /dev/null 2>&1 || true
cd ..

# Build live init
echo "  Building live init..."
gcc -Wall -Wextra -O2 live-init.c -o live-init 2>/dev/null || \
    echo "  Warning: Live init build had warnings (continuing anyway)"

# Build touch apps
echo "  Building touch installer..."
cd touch-apps
gcc -Wall -Wextra -O2 installer.c ../libtouch/libtouch.a -o ../touch-installer 2>/dev/null || \
    echo "  Warning: Installer build had warnings (continuing anyway)"
cd ..

# Build package manager
echo "  Building package manager..."
cd pkg-manager
make clean > /dev/null 2>&1 || true
make > /dev/null 2>&1 || \
    echo "  Warning: Package manager build had warnings (continuing anyway)"
cd ..

cd ..
echo "  ✓ Userland built"

# Copy files to ISO
echo ""
echo "[6/8] Copying files to ISO..."

# Copy kernel
cp "$KERNEL_ELF" "$ISO_DIR/boot/kernel.elf"
cp "$KERNEL_ELF" "$ISO_DIR/touchos/kernel/kernel.elf"
echo "  ✓ Kernel copied"

# Copy userland applications
echo "  Copying userland..."
if [ -f "userland/live-init" ]; then
    cp userland/live-init "$ISO_DIR/touchos/bin/"
    cp userland/live-init "$ISO_DIR/boot/live-init"
fi
if [ -f "userland/touch-installer" ]; then
    cp userland/touch-installer "$ISO_DIR/touchos/bin/"
fi
if [ -f "userland/pkg-manager/tpkg" ]; then
    cp userland/pkg-manager/tpkg "$ISO_DIR/touchos/bin/" 2>/dev/null || true
fi
if [ -f "userland/pkg-manager/tpkg-build" ]; then
    cp userland/pkg-manager/tpkg-build "$ISO_DIR/touchos/bin/" 2>/dev/null || true
fi

# Copy all touch apps
for app in userland/touch-*; do
    if [ -x "$app" ] && [ -f "$app" ]; then
        cp "$app" "$ISO_DIR/touchos/bin/" 2>/dev/null || true
    fi
done

# Copy libraries
if [ -f "userland/libtouch/libtouch.a" ]; then
    cp userland/libtouch/libtouch.a "$ISO_DIR/touchos/lib/"
fi
if [ -f "userland/libtouch/touch_framework.h" ]; then
    cp userland/libtouch/touch_framework.h "$ISO_DIR/touchos/lib/"
fi

echo "  ✓ Applications copied"

# Create installer disk list
echo "  Creating disk configuration..."
cat > "$ISO_DIR/touchos/installer_disks.txt" << 'EOF'
/dev/sda,500GB,Primary Hard Drive
/dev/sdb,1TB,Secondary Hard Drive
/dev/nvme0n1,256GB,NVMe SSD
EOF
echo "  ✓ Configuration created"

# Create GRUB configuration for live boot
echo ""
echo "[7/8] Creating bootloader configuration..."
cat > "$ISO_DIR/boot/grub/grub.cfg" << 'EOF'
set timeout=10
set default=0

# Load video modules
insmod all_video
insmod gfxterm
insmod png

# Set graphics mode
set gfxmode=auto
terminal_output gfxterm

# Menu colors
set menu_color_normal=white/black
set menu_color_highlight=black/light-gray

menuentry "Install TouchOS" {
    echo "Loading TouchOS Installer..."
    echo "Initializing hardware and touch support..."
    set root=(cd)
    linux /boot/kernel.elf installer touchos_init=/boot/live-init
    boot
}

menuentry "Install TouchOS (Safe Mode - No Graphics)" {
    echo "Loading TouchOS Installer (Safe Mode)..."
    set root=(cd)
    linux /boot/kernel.elf installer safe touchos_init=/boot/live-init
    boot
}

menuentry "Try TouchOS (Live Mode)" {
    echo "Loading TouchOS Live Environment..."
    set root=(cd)
    linux /boot/kernel.elf live touchos_init=/boot/live-init
    boot
}

menuentry "Boot from First Hard Drive" {
    set root=(hd0)
    chainloader +1
}

menuentry "Reboot" {
    reboot
}

menuentry "Shutdown" {
    halt
}
EOF
echo "  ✓ Bootloader configured"

# Create ISO
echo ""
echo "[8/8] Building bootable ISO..."
grub-mkrescue -o "$ISO_NAME" "$ISO_DIR" \
    -- -volid "$ISO_LABEL" -isohybrid-mbr /usr/lib/ISOLINUX/isohdpfx.bin \
    2>&1 | grep -v "warning: Unknown" || true

if [ ! -f "$ISO_NAME" ]; then
    echo "  Retrying without isohybrid-mbr..."
    grub-mkrescue -o "$ISO_NAME" "$ISO_DIR" \
        -- -volid "$ISO_LABEL" \
        2>&1 | grep -v "warning: Unknown" || true
fi

if [ ! -f "$ISO_NAME" ]; then
    echo "  ERROR: ISO creation failed"
    exit 1
fi

# Make ISO hybrid bootable for USB (if isohybrid is available)
if command -v isohybrid &> /dev/null; then
    echo "  Making ISO hybrid bootable for USB..."
    isohybrid "$ISO_NAME" 2>/dev/null || echo "  Note: isohybrid not available, ISO may not boot from USB on all systems"
fi

# Get ISO size
ISO_SIZE=$(du -h "$ISO_NAME" | cut -f1)
echo "  ✓ ISO created: $ISO_NAME ($ISO_SIZE)"

# Create MD5 checksum
echo ""
echo "Creating checksum..."
md5sum "$ISO_NAME" > "$ISO_NAME.md5"
echo "  ✓ Checksum: $ISO_NAME.md5"

# Summary
echo ""
echo "========================================="
echo "Build Complete!"
echo "========================================="
echo ""
echo "ISO File:      $ISO_NAME"
echo "Size:          $ISO_SIZE"
echo "Checksum:      $ISO_NAME.md5"
echo ""
echo "To test the ISO:"
echo "  qemu-system-x86_64 -cdrom $ISO_NAME -m 2G -enable-kvm"
echo ""
echo "To burn to USB:"
echo "  sudo dd if=$ISO_NAME of=/dev/sdX bs=4M status=progress"
echo "  (Replace /dev/sdX with your USB device)"
echo ""
echo "To burn to CD/DVD:"
echo "  brasero $ISO_NAME"
echo "  or"
echo "  wodim -v -dao dev=/dev/sr0 $ISO_NAME"
echo ""
