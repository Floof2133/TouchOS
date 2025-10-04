// kernel/kernel.c
#include <stdint.h>
#include <stddef.h>
#include "memory.h"
#include "interrupts.h"
#include "scheduler.h"
#include "vfs.h"

#define PAGE_PRESENT    0x1 	//telling CPU that it exists on the page
#define PAGE_WRITE      0x2		//without this kernel can't write anything it would be read only.
#define PAGE_SIZE       0x80    // For 2MB pages

//EFI constants
#define EfiConventionalMemory 7


//add the spinglock_t spin_lock() and spin_unlock() defs!

// Structure passed by the bootloader

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


//added this into the kernel :P
void kernel_panic(const char* message, uint32_t error_code) {
    // Disable interrupts
    asm volatile("cli");
    
    // TODO: Print error message to screen if you have graphics working
    // For now, just halt with the error code in a register
    
    // Put error code in EAX for debugging.
    asm volatile(
        "mov %0, %%eax\n"
        "hlt\n"
        : 
        : "r"(error_code)
    );
    
    // Infinite halt loop in case of NMI or other interrupts
    while(1) {
        asm volatile("hlt");
    }
}
//This is nessescary for kernel panic to work, if not it is happy go lucky and you have no idea what is going on.

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

	//I am going to add this as a check for if null is the type that is returned!
	if (pmm.bitmap == NULL) {
    // Handle error - maybe halt the system
    // For now, you could just return or loop forever but imma add a panic
		kernel_panic("PMM: Failed to allocate bitmap - no suitable memory region found", 0x01 //the last line is just a error code but we can add panic.c
    while(1) { asm volatile("hlt"); }
}
	
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

	
//for (uint64_t i = 0; i < pmm.bitmap_size * 8; i++) { 		//same here with a logic issue! Edit: I am going to swap out the pager for a dif iteration
															//this way it will use this because its cleaner.
	uint64_t num_pages = pmm.total_memory / 4096;
	for (uint64_t i = 0; i < num_pages; i++) {
	
		uint64_t byte = i / 64;
		uint64_t bit = i % 64;

		if (!(pmm.bitmap[byte] & (1ULL << bit))) {
			pmm.bitmap[byte] |= (1ULL << bit);
			pmm.free_memory -= 4096;
			spin_unlock(&pmm.lock);
			return (void*)(i * 4096); 						//fixed typo XD
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
	// Allomacate PML table
	uint64_t* pml = (uint64_t*)pmm_alloc_page();
	memset(pml, 0, 4096);


	// The first 4GB of the kernel is having an identity crisis (I don't blame them, although it a bit wierd.. Its a map identity crisis, but just be kind to them alright? They are going through a rough time)
	for (uint64_t addr = 0; addr < 0x100000000; addr += 0x200000) {	// 2MB Pages, Still a long way till they find who they truley are.
		vmm_map_page(pml, addr, addr, PAGE_PRESENT | PAGE_WRITE | PAGE_SIZE); //was missing an underscore.
	}


	// Just loading a bunch of papaer it seems like. > no its not, its a little harder than that.
	asm volatile("mov %0, %%cr3" : : "r"(pml));
	
	uint64_t cr3_value;
	asm volatile("mov %%cr3, %0" : "=r"(cr3_value));
	asm volatile("mov %0, %%cr3" : : "r"(cr3_value));
	//these last lines were outside of the function so i put them in for you. -Xansi

}	


void vmm_map_page(uint64_t* pml, uint64_t virtual_addr, uint64_t physical_addr, uint64_t flags) { //Flags?? more like.. FAGS LMAOOO :3
	uint64_t pml_idx = (virtual_addr >> 39) & 0x1FF; // 39??!? Like Hatune Miku??!? WOAH
	uint64_t pdpt_idx = (virtual_addr >> 30) & 0x1FF;
	uint64_t pd_idx = (virtual_addr >> 21) & 0x1FF;
	uint64_t pt_idx = (virtual_addr >> 12) & 0x1FF;


	// Get or create the thingymabob
	uint64_t* pdpt;
    if (!(pml[pml_idx] & PAGE_PRESENT)) {
        pdpt = (uint64_t*)pmm_alloc_page();
        memset(pdpt, 0, 4096);
        pml[pml_idx] = (uint64_t)pdpt | PAGE_PRESENT | PAGE_WRITE;
    } else {
        pdpt = (uint64_t*)(pml[pml_idx] & ~0xFFF);
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
		// sister i can't stand reading your comments -Xansi
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
	asm volatile("invlpg (%0)" : : "r"(virtual_addr));

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
