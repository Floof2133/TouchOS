; kernel/boot/boot64.asm
[BITS 64]
global _start
extern kernel_main

section .text
_start:
	; Save da boot pawamatews UwU
	mov [boot_parama.gop], rdi
	mov [boot_params.memory_map], rsi
	mov [boot_params.memory_map_size], rdx
	mov [boot_params.descriptor_size], rcx


	; Stawt da stak OwO
	mov rsp, stack_top

	; Cleaw da BeeEssEss sektion hehe
	mov rdi, __bss_start
	mov rcx, __bss_end
	sub rcx, rdi
	xor rax, rax
	rep stosb


	; Initiawize teh see pee you Feet(ures)
	call enable_see
	call enable_avx

	; Call the kernal. He lonely 3:
	mov rdi, boot_params
	call kernal_main

	; Literally kill yourself when the kernel returns
	cli
.halt:
	hlt
	jmp .halt

enabe_see:
	mov rax, cr0
	and ax, 0xFFFB ; Clear copopoporocsessor emulation
	or ax, 0x2 ; Set coopypoopyprocrastinationoscessor monitoring
	mov cr0, rax

	mov rax, cr4
	or ax, 0x600 ; set OSFXR anfd OSX(Like MacOsX :3)MMEXCPT
	mov cr4, rax
	ret ; and link hehehjeejheheh

enable_avx:
	mov rax, cr4
	or rax, 0x40000 ; Set OSX(Like MacOsX AGAIB)MMEXCPT
	mov cr4, rax


	xor rcx, rcx
	xgetbv
	or eax, 0x7 ; enable x87, which is only 20 more than 67, which is something i do not understand, like why it funny? SSE, AVX
	xsetbv
	ret

section .data
boot_params:
	.gop: dq 0
	.memory_map: dq 0
	.memory_map_size: dq 0
	.descriptor_size: dq 0
