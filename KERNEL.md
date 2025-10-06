# TouchOS Kernel Documentation

← [Back to README](README.md) | [Architecture](ARCHITECTURE.md) | [Next: Drivers](DRIVERS.md) →

## Overview

TouchOS Kernel is a 64-bit x86_64 microkernel written from scratch in C and Assembly. It provides core operating system services including memory management, process scheduling, interrupt handling, and device driver interfaces.

**File**: `kernel/kernel.c` (main kernel)
**Entry Point**: `kernel_main()`
**Architecture**: x86_64 (64-bit)
**Boot Protocol**: Multiboot2

## Kernel Features

✅ **64-bit Long Mode** - Full x86_64 support
✅ **Physical Memory Manager** - Bitmap-based page allocator
✅ **Heap Allocator** - `kmalloc()` / `kfree()` for dynamic memory
✅ **Serial Debug Output** - COM1/COM2 for kernel debugging
✅ **Multiboot2 Support** - Boots with GRUB2
🚧 **Interrupt Handling** - IDT setup (work in progress)
🚧 **Process Scheduler** - Multitasking support (planned)
🚧 **Virtual Memory Manager** - Paging and virtual address spaces (planned)

## Boot Sequence

```
1. GRUB2 loads kernel.elf
   ↓
2. boot64.asm executes (32-bit mode)
   • Checks CPU supports 64-bit
   • Sets up page tables
   • Enables long mode
   • Switches to 64-bit
   ↓
3. kernel_main() called (64-bit mode)
   • Initializes serial port
   • Initializes PMM
   • Initializes heap
   • Sets up GDT/IDT
   • Initializes drivers
   • Starts scheduler
   ↓
4. System ready for userspace
```

### Boot64.asm - Mode Transition

**Location**: `kernel/boot/boot64.asm`

**Responsibilities**:
1. **CPU Detection** - Verifies 64-bit support via CPUID
2. **Page Table Setup** - Identity maps first 512 MB (256 × 2MB pages)
3. **PAE Enable** - Physical Address Extension for 64-bit
4. **Long Mode Enable** - Sets LME bit in EFER MSR
5. **Paging Enable** - Activates paging (CR0.PG)
6. **GDT Load** - 64-bit Global Descriptor Table
7. **Jump to Long Mode** - Far jump to 64-bit code segment
8. **Call kernel_main()** - Transfer control to C kernel

**Page Table Structure**:
```
PML4 (Level 4) ──→ PDPT (Level 3) ──→ PD (Level 2) ──→ Huge 2MB Pages
   1 entry             1 entry            256 entries     (512 MB total)
```

## Memory Management

### Physical Memory Manager (PMM)

**File**: `kernel/pmm.c`, `kernel/pmm.h`

**Algorithm**: Bitmap allocator

**How it works**:
- Each bit represents one 4KB page
- 0 = free page, 1 = used page
- Linear search for first free bit

**Functions**:

```c
void pmm_init(uint64_t total_memory);    // Initialize with total RAM
void* pmm_alloc_page(void);              // Allocate 4KB page
void pmm_free_page(void* page);          // Free a page
uint64_t pmm_get_free_pages(void);       // Get free page count
```

**Example**:
```c
// Allocate a physical page
void* page = pmm_alloc_page();
if (page == NULL) {
    kernel_panic("Out of memory!", 0);
}

// Use the page...

// Free it when done
pmm_free_page(page);
```

**Memory Overhead**:
- 16 GB RAM = 4 million pages
- 4M pages ÷ 8 bits/byte = 512 KB bitmap
- Very efficient!

### Heap Allocator

**File**: `kernel/heap.c`, `kernel/heap.h`

**Algorithm**: Block-based allocator with metadata

**Features**:
- Dynamic kernel memory allocation
- Block coalescing (merges adjacent free blocks)
- Metadata tracking (size, free/used status)
- First-fit allocation strategy

**Functions**:

```c
void heap_init(void);                    // Initialize heap
void* kmalloc(size_t size);              // Allocate memory
void kfree(void* ptr);                   // Free memory
void* krealloc(void* ptr, size_t size);  // Resize allocation
```

**Usage**:

```c
// Allocate memory
char* buffer = (char*)kmalloc(1024);
if (!buffer) {
    return -ENOMEM;
}

// Use buffer...
strcpy(buffer, "Hello, kernel!");

// Free when done
kfree(buffer);
```

**Heap Layout**:
```
[Header][Data........][Header][Data...][Header][Free Space...]
  ^         ^            ^        ^
  |         |            |        |
  Metadata  User data    Metadata User data
```

## Interrupt Handling

### IDT (Interrupt Descriptor Table)

**File**: `kernel/idt.c` (planned)

**Purpose**: Maps interrupt/exception numbers to handler functions

**Setup** (planned):
```c
void idt_init(void) {
    // Set up 256 IDT entries
    for (int i = 0; i < 256; i++) {
        set_idt_gate(i, isr_stub_table[i], 0x08, 0x8E);
    }

    // Load IDT
    load_idt();
}
```

**Exception Handlers**:
- 0x00: Divide by Zero
- 0x06: Invalid Opcode
- 0x08: Double Fault
- 0x0D: General Protection Fault
- 0x0E: Page Fault
- ...and more

**IRQ Handlers**:
- IRQ 0 (INT 32): Timer
- IRQ 1 (INT 33): Keyboard
- IRQ 3/4 (INT 35/36): Serial Ports
- IRQ 14/15 (INT 46/47): IDE Controllers

### PIC (Programmable Interrupt Controller)

**Initialization** (planned):

```c
void pic_init(void) {
    // Remap PIC to avoid conflicts with CPU exceptions
    // Master PIC: IRQ 0-7 → INT 32-39
    // Slave PIC: IRQ 8-15 → INT 40-47

    outb(0x20, 0x11);  // Initialize master PIC
    outb(0xA0, 0x11);  // Initialize slave PIC
    outb(0x21, 0x20);  // Master PIC vector offset (32)
    outb(0xA1, 0x28);  // Slave PIC vector offset (40)
    // ... configuration continues
}
```

## Process Management

### Scheduler (Planned)

**Algorithm**: Round-robin with priorities

**Task Structure**:
```c
typedef struct task {
    uint64_t pid;               // Process ID
    uint64_t* page_table;       // Virtual memory space
    uint64_t rsp;               // Stack pointer
    uint64_t rip;               // Instruction pointer
    uint64_t state;             // Running/Ready/Blocked
    uint64_t priority;          // Task priority
    struct task* next;          // Next task in queue
} task_t;
```

**Functions** (planned):
```c
void scheduler_init(void);
task_t* task_create(void (*entry)(void));
void task_switch(task_t* next);
void task_yield(void);
void task_sleep(uint64_t ms);
```

**Context Switching**:
```
1. Timer interrupt fires
2. Save current task state (registers, RIP, RSP)
3. Select next task (round-robin)
4. Load next task state
5. Return from interrupt (into new task)
```

## System Calls (Planned)

### System Call Interface

**Mechanism**: `syscall` instruction (fast system call)

**Registers**:
- RAX: System call number
- RDI, RSI, RDX, R10, R8, R9: Arguments (up to 6)
- RAX: Return value

**Example**:
```c
// User space
ssize_t write(int fd, const void* buf, size_t count) {
    ssize_t ret;
    asm volatile (
        "mov $1, %%rax\n"      // syscall number (write = 1)
        "syscall\n"
        : "=a"(ret)
        : "D"(fd), "S"(buf), "d"(count)
        : "rcx", "r11", "memory"
    );
    return ret;
}
```

### System Call Table (Planned)

| Number | Name | Description |
|--------|------|-------------|
| 0 | read | Read from file descriptor |
| 1 | write | Write to file descriptor |
| 2 | open | Open file |
| 3 | close | Close file descriptor |
| 4 | stat | Get file status |
| 5 | fstat | Get file status by fd |
| ... | ... | ... |

## Virtual Memory (Planned)

### Page Table Structure

```
64-bit Virtual Address:
┌──────┬──────┬──────┬──────┬──────────────┐
│ PML4 │ PDPT │  PD  │  PT  │    Offset    │
│ 9bit │ 9bit │ 9bit │ 9bit │    12bit     │
└──────┴──────┴──────┴──────┴──────────────┘
```

**4-Level Paging**:
1. PML4 (Page Map Level 4) - 512 entries
2. PDPT (Page Directory Pointer Table) - 512 entries
3. PD (Page Directory) - 512 entries
4. PT (Page Table) - 512 entries

Each entry: 8 bytes (64-bit)
Total addressable: 256 TB

### Memory Protection

**Page Flags**:
- Present (P): Page is in memory
- Read/Write (R/W): Writable if set
- User/Supervisor (U/S): User accessible if set
- Execute Disable (XD): Prevent execution if set

**User vs Kernel Memory**:
```
0x0000_0000_0000_0000 - 0x0000_7FFF_FFFF_FFFF: User Space (128 TB)
0xFFFF_8000_0000_0000 - 0xFFFF_FFFF_FFFF_FFFF: Kernel Space (128 TB)
```

## Debugging

### Serial Output

**File**: `drivers/serial.c`

**Ports**:
- COM1: 0x3F8
- COM2: 0x2F8

**Functions**:
```c
void serial_init(void);
void serial_putchar(char c);
void serial_puts(const char* str);
void serial_printf(const char* fmt, ...);
```

**Usage**:
```c
serial_printf("Kernel starting...\n");
serial_printf("Free memory: %llu KB\n", pmm_get_free_pages() * 4);
```

**View output**:
```bash
qemu-system-x86_64 -serial stdio -kernel kernel.elf
```

### Kernel Panic

```c
void kernel_panic(const char* message, uint32_t error_code) {
    // Disable interrupts
    cli();

    // Print error
    serial_printf("\n*** KERNEL PANIC ***\n");
    serial_printf("Error: %s (code: 0x%x)\n", message, error_code);

    // Halt CPU
    while(1) {
        hlt();
    }
}
```

## Source Files

| File | Description | Lines |
|------|-------------|-------|
| `kernel/kernel.c` | Main kernel code | ~200 |
| `kernel/boot/boot64.asm` | Boot assembly (32→64 bit) | ~188 |
| `kernel/pmm.c` | Physical memory manager | ~150 |
| `kernel/pmm.h` | PMM header | ~42 |
| `kernel/heap.c` | Heap allocator | ~150 |
| `kernel/heap.h` | Heap header | ~30 |
| `kernel/linker.ld` | Linker script | ~50 |

## Build Commands

```bash
# Build kernel
make clean
make

# Output: kernel.elf (64-bit ELF executable)

# Test in QEMU
qemu-system-x86_64 -kernel kernel.elf -serial stdio
```

## Memory Map (at boot)

```
0x0000_0000  BIOS/Reserved
0x0010_0000  Kernel Image (loaded by GRUB)
0x0040_0000  PMM Bitmap
0x0050_0000  Kernel Heap (grows upward)
0x0100_0000  Free Memory (available to allocate)
...
0x4000_0000  End of RAM (example 16 GB system)
```

## Performance

| Metric | Value |
|--------|-------|
| Kernel Size | ~50 KB (compiled) |
| Boot Time | < 1 second (kernel init) |
| Memory Overhead | ~50 MB (kernel + data structures) |
| Page Allocation | O(n) - linear bitmap search |
| Heap Allocation | O(n) - first-fit search |

## Future Enhancements

- [x] Physical memory manager
- [x] Heap allocator
- [ ] Complete interrupt handling
- [ ] Process scheduler (multitasking)
- [ ] Virtual memory manager
- [ ] System call interface
- [ ] SMP (multi-core) support
- [ ] ACPI support
- [ ] Power management

## Debugging Tips

### QEMU Monitor
```bash
# Start QEMU with monitor
qemu-system-x86_64 -kernel kernel.elf -monitor stdio

# Inside QEMU monitor:
(qemu) info registers  # Show CPU registers
(qemu) x/20i $rip      # Disassemble at current instruction
(qemu) x/10gx 0x100000 # Dump memory in hex
```

### GDB Debugging
```bash
# Terminal 1: Start QEMU with GDB stub
qemu-system-x86_64 -kernel kernel.elf -s -S

# Terminal 2: Connect GDB
gdb kernel.elf
(gdb) target remote localhost:1234
(gdb) break kernel_main
(gdb) continue
```

## Common Issues

**Problem**: Triple fault / reboot loop
**Solution**: Check page table setup in boot64.asm

**Problem**: Page fault on boot
**Solution**: Verify linker script memory addresses

**Problem**: Kernel panic "Out of memory"
**Solution**: Check PMM initialization, ensure GRUB passes memory map

---

## Related Documentation

- **[Architecture](ARCHITECTURE.md)** - System architecture overview
- **[Drivers](DRIVERS.md)** - Device driver documentation
- **[Building](BUILDING.md)** - How to build the kernel
- **[Development](DEVELOPMENT.md)** - Kernel development guide

← [Back to README](README.md) | [Architecture](ARCHITECTURE.md) | [Next: Drivers](DRIVERS.md) →
