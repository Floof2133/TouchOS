; kernel/boot/boot64.asm
; Bootloader for TouchOS - handles 32-bit to 64-bit mode transition
; This is where the magic happens (switching from 32-bit to 64-bit)

MULTIBOOT_MAGIC    equ 0x1BADB002
MULTIBOOT_FLAGS    equ 0x00000000
MULTIBOOT_CHECKSUM equ -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)  ; Must add to zero or GRUB gets mad

section .multiboot
align 4
    dd MULTIBOOT_MAGIC
    dd MULTIBOOT_FLAGS
    dd MULTIBOOT_CHECKSUM

section .text
bits 32
global _start
extern kernel_main

_start:
    ; Save multiboot info (EBX has pointer to multiboot info struct from GRUB)
    mov edi, ebx
    
    ; Set up stack (we need this before calling any functions)
    mov esp, stack_top
    
    ; Check if CPU supports long mode (64-bit) - hopefully your Core i7 does lol
    call check_long_mode
    test eax, eax
    jz .no_long_mode
    
    ; Set up paging for long mode (x86_64 won't work without this)
    call setup_page_tables
    
    ; Enable PAE (Physical Address Extension) - required for 64-bit
    mov eax, cr4
    or eax, 1 << 5
    mov cr4, eax
    
    ; Load page table into CR3 (tells CPU where the page tables are)
    mov eax, p4_table
    mov cr3, eax
    
    ; Enable long mode in EFER MSR (this is the "activate 64-bit mode" switch)
    mov ecx, 0xC0000080
    rdmsr
    or eax, 1 << 8  ; Set LME bit (Long Mode Enable)
    wrmsr
    
    ; Enable paging (CR0.PG = 1)
    mov eax, cr0
    or eax, 1 << 31
    mov cr0, eax
    
    ; Load 64-bit GDT (need new segment descriptors for 64-bit)
    lgdt [gdt64.pointer]
    
    ; Far jump to 64-bit code (this actually switches us to 64-bit mode!)
    jmp gdt64.code:long_mode_start
    
.no_long_mode:
    ; CPU doesn't support 64-bit mode (this shouldn't happen on Core i7 but just in case)
    hlt
    jmp .no_long_mode

; Check if CPU supports long mode
check_long_mode:
    ; Check if CPUID is supported (flip bit 21 in EFLAGS and see if it stays flipped)
    pushfd
    pop eax
    mov ecx, eax
    xor eax, 1 << 21
    push eax
    popfd
    pushfd
    pop eax
    push ecx
    popfd
    cmp eax, ecx
    je .no_cpuid  ; If it didn't flip, no CPUID support (ancient CPU)
    
    ; Check for extended CPUID (we need this to check for long mode)
    mov eax, 0x80000000
    cpuid
    cmp eax, 0x80000001
    jb .no_long_mode  ; If less than 0x80000001, no extended CPUID
    
    ; Check if long mode is actually available
    mov eax, 0x80000001
    cpuid
    test edx, 1 << 29  ; Bit 29 = long mode support
    jz .no_long_mode
    
    mov eax, 1  ; Return 1 = success
    ret
    
.no_cpuid:
.no_long_mode:
    xor eax, eax  ; Return 0 = failure
    ret

; Set up basic page tables (identity map first 1GB using huge 2MB pages)
setup_page_tables:
    ; Map P4 table (PML4 - top level)
    mov eax, p3_table
    or eax, 0b11  ; Present + writable
    mov [p4_table], eax
    
    ; Map P3 table (PDPT - page directory pointer table)
    mov eax, p2_table
    or eax, 0b11  ; Present + writable
    mov [p3_table], eax
    
    ; Map P2 table (PD - page directory) with huge 2MB pages
    ; This is way easier than mapping individual 4KB pages
    mov ecx, 0
.map_p2_table:
    mov eax, 0x200000  ; 2MB page size
    mul ecx            ; EAX = ECX * 2MB (physical address)
    or eax, 0b10000011 ; Present + writable + huge page flag
    mov [p2_table + ecx * 8], eax
    
    inc ecx
    cmp ecx, 256       ; 256 entries * 2MB = 512GB mapped
    jne .map_p2_table
    
    ret

bits 64
long_mode_start:
    ; We're now in 64-bit mode! Let's fucking gooooo
    
    ; Load null segment selectors (64-bit doesn't really use segments but we still need to set them)
    mov ax, gdt64.data
    mov ss, ax
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Set up stack pointer (64-bit version)
    mov rsp, stack_top
    
    ; Clear BSS section (zero out all uninitialized global variables)
    mov rdi, bss_start
    mov rcx, bss_end
    sub rcx, rdi
    xor al, al
    rep stosb  ; Fast memset using x86 string operations
    
    ; Call kernel main (finally! we made it!)
    ; RDI already has multiboot info pointer from earlier
    call kernel_main
    
    ; If kernel returns (it shouldn't), halt forever
.hang:
    hlt
    jmp .hang

; 64-bit GDT (Global Descriptor Table)
; In 64-bit mode, segmentation is mostly ignored but we still need a GDT
section .rodata
gdt64:
    dq 0  ; Null descriptor (required, can't use this)
.code: equ $ - gdt64
    dq (1<<43) | (1<<44) | (1<<47) | (1<<53)  ; Code segment (executable, present, 64-bit)
.data: equ $ - gdt64
    dq (1<<44) | (1<<47)  ; Data segment (writable, present)
.pointer:
    dw $ - gdt64 - 1  ; Size of GDT - 1
    dq gdt64          ; Address of GDT

; Page tables (aligned to 4KB because x86_64 requires it)
section .bss
align 4096
p4_table:
    resb 4096  ; PML4 - top level page table
p3_table:
    resb 4096  ; PDPT - page directory pointer table
p2_table:
    resb 4096  ; PD - page directory (we use huge 2MB pages here)

bss_start:
stack_bottom:
    resb 16384  ; 16KB stack (should be plenty for kernel initialization)
stack_top:
bss_end:
