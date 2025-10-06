# TouchOS Package Manager - Quick Start Guide

## 🚀 Getting Started in 5 Minutes

### Installation

```bash
cd /home/YOUR-USER/TouchOS-build/userland/pkg-manager
make
sudo make install
```

This installs:
- `tpkg` - Package manager
- `tpkg-build` - Package builder

### Your First Package

#### Step 1: Create a Simple App

```bash
mkdir -p hello-pkg/usr/bin
cat > hello-pkg/usr/bin/hello <<'EOF'
#!/bin/sh
echo "Hello from TouchOS! 🖐️"
EOF
chmod +x hello-pkg/usr/bin/hello
```

#### Step 2: Build the Package

```bash
cd /home/YOUR-USER/TouchOS-build/userland/pkg-manager

./tpkg-build \
  -n hello \
  -v 1.0 \
  -d "Hello World for TouchOS" \
  -a "Your Name" \
  -l "MIT" \
  -s ../../hello-pkg \
  -o hello-1.0.tpkg
```

You now have `hello-1.0.tpkg`!

#### Step 3: Verify the Package

```bash
./tpkg verify hello-1.0.tpkg
```

Expected output:
```
Package verification: OK
```

#### Step 4: Upload to Repository

```bash
curl -X POST -F "package=@hello-1.0.tpkg" \
  http://159.13.54.231:8080/api/packages/upload
```

### Using the Package Manager

#### Install a Package

```bash
tpkg install hello
```

This will:
1. Download from http://159.13.54.231:8080
2. Verify checksum
3. Extract to `/usr/bin/`
4. Update database

#### List Installed Packages

```bash
tpkg list
```

#### Remove a Package

```bash
tpkg remove hello
```

## Common Tasks

### Build Package from Your Project

```bash
cd /path/to/your/project

# Build and install locally
make install DESTDIR=./pkg-root

# Create package
tpkg-build \
  -n your-project \
  -v 1.0.0 \
  -d "Your awesome project" \
  -a "Your Name" \
  -s ./pkg-root
```

### Package with Dependencies

```bash
tpkg-build \
  -n my-app \
  -v 2.0 \
  -D openssl \
  -D zlib \
  -s ./my-app-files
```

### Upload to Repository

```bash
# Method 1: During build
tpkg-build -n myapp -v 1.0 -s ./src -u http://159.13.54.231:8080

# Method 2: Manual upload
curl -X POST -F "package=@myapp-1.0.tpkg" \
  http://159.13.54.231:8080/api/packages/upload
```

## Package Structure Examples

### Example 1: Simple Binary

```
my-package/
├── usr/
│   └── bin/
│       └── myapp
└── etc/
    └── myapp.conf
```

Build:
```bash
tpkg-build -n myapp -v 1.0 -s my-package
```

### Example 2: Library Package

```
libawesome/
├── usr/
│   ├── lib/
│   │   └── libawesome.so.1.0
│   └── include/
│       └── awesome.h
└── usr/share/doc/
    └── libawesome/
        └── README
```

Build:
```bash
tpkg-build \
  -n libawesome \
  -v 1.0 \
  -d "Awesome library" \
  -l "LGPL" \
  -s libawesome
```

### Example 3: Kernel Module

```
my-driver/
└── lib/
    └── modules/
        └── my-driver.ko
```

Build:
```bash
tpkg-build \
  -n my-driver \
  -v 1.0 \
  -r \
  -i /lib/modules \
  -s my-driver
```

The `-r` flag marks that a restart is required.

## Troubleshooting

### "Package not found"
```bash
# Update repository cache first
tpkg update

# Then retry
tpkg install package-name
```

### "Checksum verification failed"
The package may be corrupted. Re-download or rebuild.

### "curl: command not found"
Install curl first:
```bash
apt-get install curl  # On Linux
```

## Repository Server

Your repository is running at:
**http://159.13.54.231:8080**

### Check Repository Status

```bash
curl http://159.13.54.231:8080/api/packages
```

### View All Packages

```bash
curl -s http://159.13.54.231:8080/api/packages | jq .
```

## Configuration

Edit `/etc/tpkg.conf`:

```conf
repo_url=http://159.13.54.231:8080
cache_dir=/var/cache/tpkg
verify_checksums=true
```

## Next Steps

- Read full documentation: `PACKAGE_MANAGER.md`
- Create your own packages
- Upload to repository
- Share with the TouchOS community!

## Tips & Tricks

### Quick Package Info

```bash
./tpkg info package-name
```

### Search Repository

```bash
tpkg search "text editor"
```

### Upgrade All Packages

```bash
tpkg upgrade-all
```

### Create from Git Repo

```bash
git clone https://github.com/user/project
cd project
make install DESTDIR=../pkg-root
cd ..
tpkg-build -n project -v 1.0 -s pkg-root
```

## File Locations

- **Binaries**: `/usr/bin/tpkg`, `/usr/bin/tpkg-build`
- **Config**: `/etc/tpkg.conf`
- **Cache**: `/var/cache/tpkg/`
- **Database**: `/var/lib/tpkg/installed.db`
- **Packages**: `/var/cache/tpkg/*.tpkg`

## Support

- Full docs: `PACKAGE_MANAGER.md`
- Repository: http://159.13.54.231:8080

---

**Happy Packaging! 📦**
