// kernel/kernel.c
#include <stdint.h>
#include <stddef.h>
#include "memory.h"
#include "interrupts.h"
#include "scheduler.h"
#include "vfs.h"

typedef struct {
	void* gap;
	void* memory_map;
	uint64_t memory_map_size;
	uint64_t descriptor_size;
} BootParams;


// are you ready for your physical? memory manager
typedef struct {
	uint64_t total_memory;
	uint64_t free_memory;
	uint64_t* bitmap;
	uint64_t bitmap_size;
	spinlock_t lock;
} PhysicalMemoryManager;

static PhysicalMemoryManager pmm = {0};

void pmm_init(void* memory_map, uint64_t map_size, uint64_t descriptor_size) {
	// Parse the parcel winner gets UEFi memorymap OwO
	uint8_t* current = (uint8_t*)memory_map;
	uint8_t* end = current + map_size;

	uint64_t highest_address = 0;
	//previously it was uint8_t, you kernel would have thought that the maximum usable adress is somewhere between 255 bytes, which the PMN would break OwO -Xansi

	while (current < end) {
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)current;

		if (desc->Type == EfiConventionalMemory) {
			pmm.free_memory += desc->NumberOfPages * 4096;
		}

		uint64_t top =  desc->PhysicalStart + (desc->NumberOfPages * 4096);
		if (top > highest_address){
			highest_address = top;
		}

		current += descriptor_size;
	}


	pmm.total_memory = highest_address;
	pmm.bitmap_size = (highest_address / 4096/ 8) + 1;

	// Allimocatios the bitchmap
	current = (uint8_t*)memory_map;
	while (current < end) {
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)current;

		if (desc->Type == EfiConventionalMemory && desc->NumberOfPages * 4096 >= pmm.bitmap_size) {
			pmm.bitmap = (uint64_t*)desc->PhysicalStart;
			memset(pmm.bitmap, 0xFF, pmm.bitmap_size); // Marks all (condoms ;3) as used initially > you forgot a semicolon :3 -Xansi
			break;
		}
		current += descriptor_size;
	}

	// Mark(iplier) free pages in the bitchmap
	current = (uint8_t*)memory_map;
	while  (current < end) {
		EFI_MEMORY_DESCRIPTOR* desc = (EFI_MEMORY_DESCRIPTOR*)current;

		if (desc->Type == EfiConventionalMemory){
			for (uint64_t i =0; i < desc->NumberOfPages; i++){ //assuming this was not intended. set the for loop to i and not "1" this would basically just make it go forever if not changed:P
				uint64_t page = (desc->PhysicalStart / 4096) + i;
				pmm.bitmap[page / 64] &= ~(1ULL << (page % 64));
			}
		}
		current += descriptor_size;
	}

}



void* pmm_alloc_page(void) {
	spin_lock(&pmm.lock);

	for (uint64_t i = 0; i < pmm.bitmap_size * 8; i++) { //same here with a logic issue!
		uint64_t byte = i / 64;
		uint64_t bit = i % 64;

		if (!(pmm.bitmap[byte] & (1ULL << bit))) {
			pmm.bitmap[byte] |= (1ULL << bit);
			pmm.free_memory -= 4096;
			spin_unlock(&pmm.lock);
			return (void*)(i * 4096); //fixed typo XD
		}
	}
	spin_unlock(&pmm.lock);
	return NULL;
}





// Virtual Memory Manager, if your too pussy to install on bare metal (even tho vm's have nothing to do with Virtual Memory Manager
typedef struct  {
	uint64_t* pml4; // Page Map Lvl 4, Congrats on the level up little buddy!
} VirtualMemoryManager;

void vmm_init(void) {
	// Allomacate PML4 table
	uint64_t* pml4 = (uint64_t*)pmm_alloc_page();
	memset(pml4, 0, 4096);


	// The first 4GB of the kernel is having an identity crisis (I don't blame them, although it a bit wierd.. Its a map identity crisis, but just be kind to them alright? They are going through a rough time)
	for (uint64_t addr = 0; addr < 0x100000000; addr += 0x200000) {	// 2MB Pages, Still a long way till they find who they truley are.
		vmm_map_page(pml4, addr, addr, PAGE_PRESENT | PAGE_WRITE | PAGE SIZE);
	}


	// Just loading a bunch of papaer it seems like.
	asm volatile("mov %0, %%cr3" : : "r"(pml4));
}


void vmm_map_page(uint64_t* pml4, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) { //Flags?? more like.. FAGS LMAOOO :3
	uint64_t pml_idx = (virtual_addr >> 39) & 0x1FF; // 39??!? Like Hatune Miku??!? WOAH
	uint64_t pdpt_idx = (virtual_addr >> 30) & 0x1FF;
	uint64_t pd_idx = (virtual_addr >> 21) & 0x1FF;
	uint64_t pt_idx = (virtual_addr >> 12) & 0x1FF;


	// Get or create the thingymabob
	uint64_t* pdpt;
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        pdpt = (uint64_t*)pmm_alloc_page();
        memset(pdpt, 0, 4096);
        pml4[pml4_idx] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITE;
    } else {
        pdpt = (uint64_t*)(pml4[pml4_idx] & ~0xFFF);
    }

	// Get or create the POLICE DEPARTMENT WOOP WOOP DAS THE SOUND OF THE POLICE hehehehhe
	uint64_t* pd;
    	if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        	pd = (uint64_t*)pmm_alloc_page();
        	memset(pd, 0, 4096);
       	 	pdpt[pdpt_idx] = (uint64_t)pd | PAGE_PRESENT | PAGE_WRITE;
    	} else {
        	pd = (uint64_t*)(pdpt[pdpt_idx] & ~0xFFF);
    	}


	// Hanleing just a little amout of papaer, roughly 2mb
	if (flags & PAGE_SIZE) {
        	pd[pd_idx] = physical_addr | flags;
	} else {
		// Pee in some Tea!!  (PT)
		uint64_t* pt;
        	if (!(pd[pd_idx] & PAGE_PRESENT)) {
            		pt = (uint64_t*)pmm_alloc_page();
            		memset(pt, 0, 4096);
            		pd[pd_idx] = (uint64_t)pt | PAGE_PRESENT | PAGE_WRITE;
        	} else {
            		pt = (uint64_t*)(pd[pd_idx] & ~0xFFF);
        	}

		// Map out the seven seas, well the four seas. Map 4KP page...
		pt[pt_idx] = physical_addr | flags;
	}


	// Flush the Toilet Bowl (TLB) for this sheet of toilet paper (page)
	asm volatitle("invlpg (%0)" : : "r"(virtual_addr));

}

void kernel_main(BootParams* params) {
	// Initialise the physical memory manager. (I dont have anything funny for this 3:)
	pmm_init(params->memory_map, params->memory_map_size, params->descriptor_size);

	// Initialise Virtual memory
	vmm_init();

	// Set up GDT and IDT
	gdt_init();
	idt_init();

	// Initialise  interrupt controller
	pic_init();
	apic_init();

	// Init scheduler
	scheduler_init();

	// init Devce drivers
	pci_scan();
	usb_init();

	// Init graphics
	graphics_init(params->gop);

	// Init filesystem
	vfs_init();
	initrd_load();

	// Start firs user process
	process_create("/sbin/init");

	// Enable interuptions and start sceduling
	sti();
	scheduler_start();
}

// Kernel for TouchOS developed by - Lexii (Floof<3) and Xansi
//Things may change in the near future (they probably will)
