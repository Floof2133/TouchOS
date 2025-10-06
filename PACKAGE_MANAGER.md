# TouchOS Package Manager (tpkg) Documentation

## Overview

The TouchOS Package Manager (`tpkg`) is a custom package management system designed for TouchOS. It uses the `.tpkg` package format and integrates with the TouchOS package repository.

**Repository URL:** http://159.13.54.231:8080
**API Endpoint:** http://159.13.54.231:8080/api/packages

## Features

- ✅ Custom `.tpkg` package format with metadata and compressed files
- ✅ Package installation, removal, and upgrades
- ✅ Dependency resolution
- ✅ SHA256 checksum verification
- ✅ Package building from source directories
- ✅ Repository integration for uploads/downloads
- ✅ Networking stack for HTTP package downloads

## Package Format (.tpkg)

### Structure

```
┌──────────────────────────────────────┐
│ Header (512 bytes)                   │
│  - Magic: "TPKG" (4 bytes)          │
│  - Version: 1 (4 bytes)             │
│  - Metadata size (8 bytes)          │
│  - Data size (8 bytes)              │
│  - SHA256 checksum (32 bytes)       │
│  - Reserved (456 bytes)             │
├──────────────────────────────────────┤
│ Metadata (JSON, variable length)     │
│  - Package name, version, etc.       │
│  - Dependencies                      │
│  - Install path                      │
├──────────────────────────────────────┤
│ Data (tar.gz, variable length)       │
│  - Compressed package files          │
└──────────────────────────────────────┘
```

### Metadata Fields

```json
{
  "name": "package-name",
  "version": "1.0.0",
  "description": "Package description",
  "author": "Author Name",
  "license": "MIT",
  "architecture": "x86_64",
  "install_path": "/usr/local",
  "requires_restart": false,
  "build_timestamp": 1234567890,
  "dependencies": ["dep1", "dep2"]
}
```

## Installation

### Building the Tools

```bash
cd userland/pkg-manager
make
sudo make install
```

This will install:
- `/usr/bin/tpkg` - Package manager CLI
- `/usr/bin/tpkg-build` - Package builder
- `/etc/tpkg.conf` - Configuration file

## Usage

### Package Manager (tpkg)

#### Install a Package

```bash
tpkg install vim
```

Automatically:
- Downloads from repository
- Verifies checksum
- Resolves and installs dependencies
- Extracts to install path
- Updates package database

#### Remove a Package

```bash
tpkg remove vim
```

#### Upgrade a Package

```bash
tpkg upgrade vim
```

#### Upgrade All Packages

```bash
tpkg upgrade-all
```

#### Update Repository Cache

```bash
tpkg update
```

#### List Installed Packages

```bash
tpkg list
```

Output:
```
Installed packages (3):
  vim (8.2)
  curl (7.68)
  ncurses (6.2)
```

#### Search for Packages

```bash
tpkg search editor
```

#### Show Package Information

```bash
tpkg info vim
```

#### Verify Package Integrity

```bash
tpkg verify package-1.0.tpkg
```

### Package Builder (tpkg-build)

#### Create a Package

Basic example:
```bash
tpkg-build -n myapp -v 1.0 -d "My Application" -s ./myapp-src
```

Full example with all options:
```bash
tpkg-build \
  -n myapp \
  -v 1.0.5 \
  -d "My awesome application" \
  -a "Your Name" \
  -l "MIT" \
  -s ./myapp-build \
  -o myapp-1.0.5.tpkg \
  -i /usr/local \
  -D libc \
  -D openssl \
  -r
```

Options:
- `-n` : Package name (required)
- `-v` : Version (required)
- `-d` : Description
- `-a` : Author name
- `-l` : License (MIT, GPL, etc.)
- `-s` : Source directory to package
- `-o` : Output .tpkg filename
- `-i` : Installation path
- `-D` : Add dependency (use multiple times)
- `-r` : Requires system restart
- `-u` : Upload to repository after building

#### Example: Building Hello World

```bash
# Create package structure
mkdir -p hello-pkg/usr/bin
echo '#!/bin/sh' > hello-pkg/usr/bin/hello
echo 'echo "Hello from TouchOS!"' >> hello-pkg/usr/bin/hello
chmod +x hello-pkg/usr/bin/hello

# Build package
tpkg-build \
  -n hello \
  -v 1.0 \
  -d "Hello World application" \
  -a "TouchOS Team" \
  -l "MIT" \
  -s hello-pkg \
  -o hello-1.0.tpkg

# Verify
tpkg verify hello-1.0.tpkg
```

#### Upload Package to Repository

```bash
# Build and upload in one command
tpkg-build -n myapp -v 1.0 -s ./src -u http://159.13.54.231:8080

# Or upload existing package
curl -X POST -F "package=@myapp-1.0.tpkg" \
  http://159.13.54.231:8080/api/packages/upload
```

**Important:** Only `.tpkg` files can be uploaded to the repository. The server will reject non-.tpkg files.

## Configuration

### Configuration File (`/etc/tpkg.conf`)

```conf
# Repository URL
repo_url=http://159.13.54.231:8080

# Package cache directory
cache_dir=/var/cache/tpkg

# Installed packages database
db_file=/var/lib/tpkg/installed.db

# Installation root
install_root=/

# Verify package checksums
verify_checksums=true
```

### Directory Structure

```
/var/cache/tpkg/          # Downloaded packages
/var/lib/tpkg/            # Package database
  └── installed.db        # List of installed packages
```

## Package Repository API

### Get Package List

```bash
curl http://159.13.54.231:8080/api/packages
```

Response:
```json
{
  "repository": "TouchOS Official Repository",
  "version": "1.0",
  "packages": [
    {
      "name": "vim",
      "version": "8.2",
      "description": "Vi IMproved text editor",
      "download_url": "http://159.13.54.231:8080/packages/vim-8.2.tpkg",
      "size": 1234567,
      "checksum": "abc123..."
    }
  ]
}
```

### Download Package

```bash
curl -O http://159.13.54.231:8080/packages/vim-8.2.tpkg
```

### Upload Package

```bash
curl -X POST -F "package=@myapp-1.0.tpkg" \
  http://159.13.54.231:8080/api/packages/upload
```

**Requirements:**
- File must have `.tpkg` extension
- File must be valid .tpkg format
- Checksum will be verified

## Networking

The package manager includes a basic TCP/IP networking stack:

### Components

1. **Ethernet Layer** - Frame handling
2. **IP Layer** - IPv4 packet routing
3. **TCP** - Reliable connections for HTTP
4. **UDP** - Datagram support
5. **ARP** - Address resolution
6. **HTTP Client** - Package downloads

### Network Configuration

```c
// In kernel initialization
net_init();

netif_t* eth0 = netif_create("eth0");
netif_set_addr(eth0,
    ip_from_string("192.168.1.100"),
    ip_from_string("255.255.255.0"),
    ip_from_string("192.168.1.1"));
netif_set_mac(eth0, mac_address);
netif_up(eth0);
```

## Development Workflow

### Creating a New Package

1. **Develop your application**
   ```bash
   mkdir myapp-src
   # Add your files to myapp-src/
   ```

2. **Create package structure**
   ```bash
   mkdir -p myapp-pkg/usr/local/bin
   cp myapp-src/myapp myapp-pkg/usr/local/bin/
   ```

3. **Build package**
   ```bash
   tpkg-build -n myapp -v 1.0 -s myapp-pkg
   ```

4. **Test locally**
   ```bash
   tpkg verify myapp-1.0.tpkg
   sudo tpkg install myapp-1.0.tpkg
   ```

5. **Upload to repository**
   ```bash
   tpkg-build -n myapp -v 1.0 -s myapp-pkg \
     -u http://159.13.54.231:8080
   ```

## Examples

### Example 1: Simple Utility

```bash
# Create a simple backup script package
mkdir -p backup-pkg/usr/local/bin
cat > backup-pkg/usr/local/bin/backup <<'EOF'
#!/bin/sh
tar czf /backup/backup-$(date +%Y%m%d).tar.gz $HOME
EOF
chmod +x backup-pkg/usr/local/bin/backup

tpkg-build \
  -n backup-tool \
  -v 1.0 \
  -d "Simple backup utility" \
  -a "Admin" \
  -l "GPL" \
  -s backup-pkg
```

### Example 2: Application with Dependencies

```bash
tpkg-build \
  -n web-server \
  -v 2.0 \
  -d "Lightweight web server" \
  -D openssl \
  -D zlib \
  -s web-server-src \
  -i /opt/webserver
```

### Example 3: Kernel Module

```bash
tpkg-build \
  -n my-driver \
  -v 1.0 \
  -d "Custom hardware driver" \
  -r \
  -i /lib/modules/$(uname -r) \
  -s driver-src
```

## Troubleshooting

### Package Installation Fails

```bash
# Update repository cache
tpkg update

# Verify package
tpkg verify package.tpkg

# Check logs
cat /var/log/tpkg.log
```

### Network Issues

```bash
# Test connectivity
ping 159.13.54.231

# Test repository
curl http://159.13.54.231:8080/api/packages
```

### Dependency Conflicts

```bash
# List installed packages
tpkg list

# Check specific package info
tpkg info problematic-package
```

## Advanced Features

### Custom Repository

Set up your own repository:

```bash
# Update config
echo "repo_url=http://your-server:8080" > /etc/tpkg.conf

# Update cache
tpkg update
```

### Package Signing (Future)

```bash
# Sign package (planned)
tpkg-build -n myapp -v 1.0 -s src --sign

# Verify signature (planned)
tpkg verify --check-signature myapp-1.0.tpkg
```

## Contributing

To contribute packages to the TouchOS repository:

1. Build your package using `tpkg-build`
2. Test thoroughly on TouchOS
3. Upload to repository
4. Submit package description to maintainers

## License

TouchOS Package Manager is part of TouchOS.
See main repository for license information.

## Support

- Repository Issues: http://159.13.54.231:8080 (If it can't connect, its down)
- TouchOS GitHub: https://github.com/Floof2133/TouchOS
- Documentation: This file

---

**Made by Floof<3** 💙
