# TouchOS Userland Makefile
# Builds all touch-optimized applications

CC = gcc
CFLAGS = -Wall -Wextra -O2 -I.
LDFLAGS = -lm -lutil

# Directories
LIBTOUCH_DIR = libtouch
TOUCH_APPS_DIR = touch-apps
TOUCH_DESKTOP_DIR = touch-desktop
PKG_MANAGER_DIR = pkg-manager

# Library
LIBTOUCH = $(LIBTOUCH_DIR)/libtouch.a
LIBTOUCH_SRC = $(LIBTOUCH_DIR)/touch_framework.c
LIBTOUCH_OBJ = $(LIBTOUCH_DIR)/touch_framework.o

# Touch Applications
TOUCH_APPS = \
	touch-settings \
	touch-file-manager \
	touch-terminal \
	touch-system-monitor \
	touch-installer

# Touch Desktop
TOUCH_DESKTOP = touch-launcher

# Package Manager
PKG_MANAGER_APPS = tpkg tpkg-build tpkg-touch-gui

# All binaries
ALL_BINS = $(TOUCH_APPS) $(TOUCH_DESKTOP) $(PKG_MANAGER_APPS)

# Install paths
PREFIX ?= /usr
BINDIR = $(PREFIX)/bin

# ============================================================================
# Main Targets
# ============================================================================

.PHONY: all clean install libtouch apps desktop pkg-manager

all: libtouch apps desktop pkg-manager

libtouch: $(LIBTOUCH)

apps: libtouch $(TOUCH_APPS)

desktop: libtouch $(TOUCH_DESKTOP)

pkg-manager: $(PKG_MANAGER_APPS)

# ============================================================================
# Library Build
# ============================================================================

$(LIBTOUCH): $(LIBTOUCH_OBJ)
	ar rcs $@ $^
	ranlib $@

$(LIBTOUCH_OBJ): $(LIBTOUCH_SRC) $(LIBTOUCH_DIR)/touch_framework.h
	$(CC) $(CFLAGS) -c $< -o $@

# ============================================================================
# Touch Applications
# ============================================================================

touch-settings: $(TOUCH_APPS_DIR)/settings.c $(LIBTOUCH)
	$(CC) $(CFLAGS) -o $@ $< $(LIBTOUCH) $(LDFLAGS)

touch-file-manager: $(TOUCH_APPS_DIR)/file-manager.c $(LIBTOUCH)
	$(CC) $(CFLAGS) -o $@ $< $(LIBTOUCH) $(LDFLAGS)

touch-terminal: $(TOUCH_APPS_DIR)/terminal.c $(LIBTOUCH)
	$(CC) $(CFLAGS) -o $@ $< $(LIBTOUCH) $(LDFLAGS)

touch-system-monitor: $(TOUCH_APPS_DIR)/system-monitor.c $(LIBTOUCH)
	$(CC) $(CFLAGS) -o $@ $< $(LIBTOUCH) $(LDFLAGS)

touch-installer: $(TOUCH_APPS_DIR)/installer.c $(LIBTOUCH)
	$(CC) $(CFLAGS) -o $@ $< $(LIBTOUCH) $(LDFLAGS)

# ============================================================================
# Touch Desktop
# ============================================================================

touch-launcher: $(TOUCH_DESKTOP_DIR)/touch-launcher.c $(LIBTOUCH)
	$(CC) $(CFLAGS) -o $@ $< $(LIBTOUCH) $(LDFLAGS)

# ============================================================================
# Package Manager
# ============================================================================

tpkg: $(PKG_MANAGER_DIR)/tpkg-cli.o $(PKG_MANAGER_DIR)/tpkg.o
	cd $(PKG_MANAGER_DIR) && $(MAKE) tpkg
	cp $(PKG_MANAGER_DIR)/tpkg .

tpkg-build: $(PKG_MANAGER_DIR)/tpkg-build.o $(PKG_MANAGER_DIR)/tpkg.o
	cd $(PKG_MANAGER_DIR) && $(MAKE) tpkg-build
	cp $(PKG_MANAGER_DIR)/tpkg-build .

tpkg-touch-gui: $(PKG_MANAGER_DIR)/tpkg-touch-gui.o $(PKG_MANAGER_DIR)/tpkg.o
	cd $(PKG_MANAGER_DIR) && $(MAKE) tpkg-touch-gui
	cp $(PKG_MANAGER_DIR)/tpkg-touch-gui .

# ============================================================================
# Installation
# ============================================================================

install: all
	@echo "Installing Touch Applications..."
	install -d $(DESTDIR)$(BINDIR)
	install -m 755 touch-settings $(DESTDIR)$(BINDIR)/
	install -m 755 touch-file-manager $(DESTDIR)$(BINDIR)/
	install -m 755 touch-terminal $(DESTDIR)$(BINDIR)/
	install -m 755 touch-system-monitor $(DESTDIR)$(BINDIR)/
	install -m 755 touch-desktop $(DESTDIR)$(BINDIR)/
	install -m 755 tpkg $(DESTDIR)$(BINDIR)/
	install -m 755 tpkg-build $(DESTDIR)$(BINDIR)/
	install -m 755 tpkg-touch-gui $(DESTDIR)$(BINDIR)/
	@echo "Installation complete!"
	@echo ""
	@echo "Available touch applications:"
	@echo "  - touch-desktop        Main launcher/desktop"
	@echo "  - touch-settings       System settings"
	@echo "  - touch-file-manager   File browser"
	@echo "  - touch-terminal       Terminal emulator"
	@echo "  - touch-system-monitor System monitor"
	@echo "  - tpkg-touch-gui       Package manager GUI"
	@echo ""
	@echo "To start the touch desktop:"
	@echo "  touch-desktop"

uninstall:
	rm -f $(DESTDIR)$(BINDIR)/touch-settings
	rm -f $(DESTDIR)$(BINDIR)/touch-file-manager
	rm -f $(DESTDIR)$(BINDIR)/touch-terminal
	rm -f $(DESTDIR)$(BINDIR)/touch-system-monitor
	rm -f $(DESTDIR)$(BINDIR)/touch-desktop
	rm -f $(DESTDIR)$(BINDIR)/tpkg
	rm -f $(DESTDIR)$(BINDIR)/tpkg-build
	rm -f $(DESTDIR)$(BINDIR)/tpkg-touch-gui

# ============================================================================
# Cleanup
# ============================================================================

clean:
	rm -f $(TOUCH_APPS)
	rm -f $(TOUCH_DESKTOP)
	rm -f $(PKG_MANAGER_APPS)
	rm -f $(LIBTOUCH_OBJ)
	rm -f $(LIBTOUCH)
	cd $(PKG_MANAGER_DIR) && $(MAKE) clean || true
	@echo "Clean complete"

# ============================================================================
# Help
# ============================================================================

help:
	@echo "TouchOS Userland Build System"
	@echo ""
	@echo "Targets:"
	@echo "  all              - Build everything (default)"
	@echo "  libtouch         - Build touch framework library"
	@echo "  apps             - Build all touch applications"
	@echo "  desktop          - Build touch desktop/launcher"
	@echo "  pkg-manager      - Build package manager tools"
	@echo "  install          - Install all binaries to $(BINDIR)"
	@echo "  uninstall        - Remove installed binaries"
	@echo "  clean            - Remove built binaries and objects"
	@echo "  help             - Show this help message"
	@echo ""
	@echo "Applications built:"
	@echo "  - touch-desktop        Main touch launcher (home screen)"
	@echo "  - touch-settings       Touch-optimized settings app"
	@echo "  - touch-file-manager   Touch file browser"
	@echo "  - touch-terminal       Terminal with on-screen keyboard"
	@echo "  - touch-system-monitor Task manager / system info"
	@echo "  - tpkg                 Package manager CLI"
	@echo "  - tpkg-build           Package builder"
	@echo "  - tpkg-touch-gui       Package manager touch GUI"
	@echo ""
	@echo "Usage:"
	@echo "  make              # Build all"
	@echo "  make install      # Install to system"
	@echo "  make clean        # Clean build files"
