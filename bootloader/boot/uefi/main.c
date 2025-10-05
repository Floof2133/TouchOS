#include <efi.h>
#include <efilib.h>
#include <elf.h>

typedef struct {
	void* base_address;
	UINT64 size;
	UINT64 entry_point;
} KernelInfo;

EFI_STATUS load_kernel(EFI_HANDLE ImageHandle, KernelInfo* kernel_info) {
	EFI_STATUS status;
	EFI_FILE_PROTOCOL* root_fs;
	EFI_FILE_PROTOCOL* kernel_file;

	//  open da root filesystem :3
	EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
	//status = uefi_call_wrapper(kernel_file->Read, 3, ImageHandle, &gEfiSimpleFileSystemProtocolGuid, &fs);		if it stops working its my fault.
	status = uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &size, &elf_header);
	
	if (EFI_ERROR(status)) return status;

	status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &root_fs);
	if (EFI_ERROR(status))  return status;

	// Load teh kurnal file tee hee
	status = uefi_call_wrapper(root_fs->Open, 5, root_fs, &kernel_file, L"\\kernel.elf", EFI_FILE_MODE_READ, 0);
	if (EFI_ERROR(STATUS)) return status;

	// give the ELF some head ;3 and "load" the segments
	Elf64_Ehdr elf_header; //there was supposed to be no i. -Xansi
	UINTN size = sizeof(elf_header);
	status = uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &elf_header)

	// MAKE SURE MR ELF HAS MAGIC
	if  (memcmp(elf_header.e_ident, ELFMAG, SELFMAG) !=0){
		return EFI_LOAD_ERROR;
	}

	// Give the program "head"ers
	Elf64_Phdr* phdrs;
	size = elf_header.e_phnum * sizeof(Elf64_Phdr);
	status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, size, &phdrs);

	uefi_call_wrapper(kernel_file->SetPosition, 2, kernel_file, elf_header.e_phoff);
	uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &size, phdrs);

	// "load" SEgMENt into memory
	for (int i = 0; i < elf_header.e_phnum; i++) {
		if (phdrs[i].p_type == PT_LOAD) {
			void* segment; //no SEgMENt 3:
			UINTN pages = (phdrs[i].p_memsz + 4095) / 4096;
			status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, EfiLoaderData, pages, &phdrs[i].p_vaddr);
			uefi_call_wrapper(kernel_file->SetPosition, 2, kernel_file, phdrs[i].p_offset);
			size = phdrs[i].p_filesz;
			uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &size, (void*)phdrs[i].p_vaddr);

			// No No BBS sectio, you aint welcome  3:
			if (phdrs[i].p_memsz > phdrs[i].p_filesz) {
				memset((void*)(phdrs[i].p_vaddr + phdrs[i].p_filesz), 0, phdrs[i].p_memsz - phdrs[i].p_filesz);
			}

		}
	}


	kernel_info->entry_point = elf_header.e_entry;
	return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
	InitializeLib(ImageHandle, SystemTable);


	// LET THERE BE LIGHT! (Graphics mode.. Its 4am, give me a break)
	EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
	EFI_STATUS status = uefi_call_wrapper(BS->LocateProtocol, 3, &gEfiGraphicsOutputProtocolGuid, NULL, &gop); //fixed typo

	// Im the map, Im the map, Im the map, Im the map, IM THE (Memory) MAP!!!!
	UINTN memory_map_size = 0;
	EFI_MEMORY_DESCRIPTOR* memory_map = NULL
	UINTN map_key, descriptor_size;
	UINT32 descriptor_version;

	status =  uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, memory_map, &map_key, &descriptor_size, &descriptor_version);
	memory_map_size += 2 * descriptor_size;
	status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, memory_map_size, &memory_map);
	status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, memory_map, &map-Key, &descriptor_size, &descriptor_version);



	// Busting a fat load on the kernel. (Loading kernel)
	KernalInfo kernal_info;
	load_kernal(ImageHandle, &kernel_info);

	// Wiping the load off the kernal's face, because im a nice person. (Exit boot services and jumt to kernal, I couldnt think of anything funny)
	status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, map_key);

	// JUMP DA FUQ UP to the kernel entry point
	void (*kernel_start)(EFI_GRAPICS_OUTPUT_PROTOCOL*, EFI_MEMORY_DESCRIPTOR*, UINTN, UINTN) =
		(void*)kernel_info.entry_point;
	kernel_start(gop, memory_map, memory_map_size, descriptor_size);

	return EFI_SUCCESS
}



