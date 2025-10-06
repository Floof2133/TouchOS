# Makefile for TouchOS Package Manager
# Builds tpkg tools for package management

CC = gcc
CFLAGS = -Wall -Wextra -O2 -std=c11
LDFLAGS =

# Binaries
BINARIES = tpkg tpkg-build tpkg-touch-gui

all: $(BINARIES)

# Package manager CLI
tpkg: tpkg-cli.o tpkg.o
	$(CC) $(LDFLAGS) -o tpkg tpkg-cli.o tpkg.o

tpkg-cli.o: tpkg-cli.c tpkg.h
	$(CC) $(CFLAGS) -c tpkg-cli.c -o tpkg-cli.o

# Package builder
tpkg-build: tpkg-build.o tpkg.o
	$(CC) $(LDFLAGS) -o tpkg-build tpkg-build.o tpkg.o

tpkg-build.o: tpkg-build.c tpkg.h
	$(CC) $(CFLAGS) -c tpkg-build.c -o tpkg-build.o

# Shared library
tpkg.o: tpkg.c tpkg.h
	$(CC) $(CFLAGS) -c tpkg.c -o tpkg.o

# Touch GUI
tpkg-touch-gui: tpkg-touch-gui.o tpkg.o
	$(CC) $(LDFLAGS) -o tpkg-touch-gui tpkg-touch-gui.o tpkg.o

tpkg-touch-gui.o: tpkg-touch-gui.c tpkg.h
	$(CC) $(CFLAGS) -c tpkg-touch-gui.c -o tpkg-touch-gui.o

# Install binaries
install: $(BINARIES)
	install -d $(DESTDIR)/usr/bin
	install -m 755 tpkg $(DESTDIR)/usr/bin/
	install -m 755 tpkg-build $(DESTDIR)/usr/bin/
	install -m 755 tpkg-touch-gui $(DESTDIR)/usr/bin/
	install -d $(DESTDIR)/etc
	echo "repo_url=http://159.13.54.231:8080" > $(DESTDIR)/etc/tpkg.conf
	echo "cache_dir=/var/cache/tpkg" >> $(DESTDIR)/etc/tpkg.conf
	install -d $(DESTDIR)/var/cache/tpkg
	install -d $(DESTDIR)/var/lib/tpkg

# Clean build artifacts
clean:
	rm -f *.o $(BINARIES)

# Create example package
example: tpkg-build
	mkdir -p example-pkg/usr/bin
	echo '#!/bin/sh' > example-pkg/usr/bin/hello
	echo 'echo "Hello from TouchOS!"' >> example-pkg/usr/bin/hello
	chmod +x example-pkg/usr/bin/hello
	./tpkg-build -n hello -v 1.0 -d "Example Hello World" \
		-a "TouchOS Team" -l "MIT" -s example-pkg -o hello-1.0.tpkg
	rm -rf example-pkg

.PHONY: all clean install example
