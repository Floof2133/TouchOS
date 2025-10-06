# TouchOS Development Guide

← [Back to README](README.md) | [Building](BUILDING.md) | [Next: Contributing](CONTRIBUTING.md) →

## Project Structure

```
TouchOS-build/
├── kernel/              # Kernel source
│   ├── kernel.c        # Main kernel
│   ├── pmm.c           # Physical memory manager
│   ├── heap.c          # Heap allocator
│   └── boot/           # Boot code
├── drivers/            # Device drivers
│   ├── serial.c        # Serial port
│   ├── usb/            # USB stack
│   └── input/          # Input devices
├── userland/           # User-space applications
│   ├── libtouch/       # Touch framework
│   ├── touch-apps/     # Touch applications
│   └── pkg-manager/    # Package manager
├── build-iso.sh        # ISO builder
└── Makefile            # Build system
```

## Coding Standards

### C Style

```c
// Functions: snake_case
void my_function(int param);

// Types: snake_case_t
typedef struct my_struct {
    int field;
} my_struct_t;

// Constants: UPPER_CASE
#define MY_CONSTANT 42

// Variables: snake_case
int my_variable = 0;
```

### Comments

```c
// Single-line comments for brief explanations

/*
 * Multi-line comments for detailed
 * documentation
 */
```

## Debugging

### Serial Debugging

```c
#include "drivers/serial.h"

serial_printf("Debug: value = %d\n", value);
```

### QEMU + GDB

```bash
# Terminal 1
qemu-system-x86_64 -kernel kernel.elf -s -S

# Terminal 2
gdb kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

## Adding Features

### New Application

1. Create `userland/touch-apps/myapp.c`
2. Include `../libtouch/touch_framework.h`
3. Add to `userland/Makefile`
4. Build and test

### New Driver

1. Create `drivers/mydriver/mydriver.c`
2. Implement init, read, write functions
3. Register with kernel
4. Add to build system

## Testing

- Test in QEMU first
- Verify on real hardware
- Check memory leaks
- Profile performance

← [Back to README](README.md) | [Building](BUILDING.md) | [Next: Contributing](CONTRIBUTING.md) →
