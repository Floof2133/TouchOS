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
    
    // Get filesystem protocol
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL* fs;
    status = uefi_call_wrapper(BS->HandleProtocol, 3, ImageHandle, 
                              &gEfiSimpleFileSystemProtocolGuid, &fs);
    if (EFI_ERROR(status)) return status;
    
    // Open root volume
    status = uefi_call_wrapper(fs->OpenVolume, 2, fs, &root_fs);
    if (EFI_ERROR(status)) return status;
    
    // Open kernel file
    status = uefi_call_wrapper(root_fs->Open, 5, root_fs, &kernel_file, 
                              L"\\kernel.elf", EFI_FILE_MODE_READ, 0);
    if (EFI_ERROR(status)) return status;
    
    // Read ELF header
    Elf64_Ehdr elf_header;
    UINTN size = sizeof(elf_header);
    status = uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &size, &elf_header);
    if (EFI_ERROR(status)) return status;
    
    // Verify ELF magic
    if (memcmp(elf_header.e_ident, ELFMAG, SELFMAG) != 0) {
        return EFI_LOAD_ERROR;
    }
    
    // Read program headers
    Elf64_Phdr* phdrs;
    size = elf_header.e_phnum * sizeof(Elf64_Phdr);
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, size, &phdrs);
    if (EFI_ERROR(status)) return status;
    
    uefi_call_wrapper(kernel_file->SetPosition, 2, kernel_file, elf_header.e_phoff);
    uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &size, phdrs);
    
    // Load segments
    for (int i = 0; i < elf_header.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            UINTN pages = (phdrs[i].p_memsz + 4095) / 4096;
            EFI_PHYSICAL_ADDRESS addr = phdrs[i].p_vaddr;
            
            status = uefi_call_wrapper(BS->AllocatePages, 4, AllocateAddress, 
                                      EfiLoaderData, pages, &addr);
            if (EFI_ERROR(status)) continue;
            
            uefi_call_wrapper(kernel_file->SetPosition, 2, kernel_file, phdrs[i].p_offset);
            size = phdrs[i].p_filesz;
            uefi_call_wrapper(kernel_file->Read, 3, kernel_file, &size, (void*)phdrs[i].p_vaddr);
            
            // Zero BSS section
            if (phdrs[i].p_memsz > phdrs[i].p_filesz) {
                memset((void*)(phdrs[i].p_vaddr + phdrs[i].p_filesz), 0, 
                       phdrs[i].p_memsz - phdrs[i].p_filesz);
            }
        }
    }
    
    kernel_info->entry_point = elf_header.e_entry;
    return EFI_SUCCESS;
}

EFI_STATUS EFIAPI efi_main(EFI_HANDLE ImageHandle, EFI_SYSTEM_TABLE* SystemTable) {
    InitializeLib(ImageHandle, SystemTable);
    
    // Get graphics output protocol
    EFI_GRAPHICS_OUTPUT_PROTOCOL* gop;
    EFI_STATUS status = uefi_call_wrapper(BS->LocateProtocol, 3, 
                                         &gEfiGraphicsOutputProtocolGuid, NULL, &gop);
    if (EFI_ERROR(status)) {
        Print(L"Failed to get GOP\n");
        return status;
    }
    
    // Get memory map
    UINTN memory_map_size = 0;
    EFI_MEMORY_DESCRIPTOR* memory_map = NULL;
    UINTN map_key, descriptor_size;
    UINT32 descriptor_version;
    
    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, memory_map, 
                              &map_key, &descriptor_size, &descriptor_version);
    memory_map_size += 2 * descriptor_size;
    status = uefi_call_wrapper(BS->AllocatePool, 3, EfiLoaderData, 
                              memory_map_size, &memory_map);
    status = uefi_call_wrapper(BS->GetMemoryMap, 5, &memory_map_size, memory_map, 
                              &map_key, &descriptor_size, &descriptor_version);
    
    // Load kernel
    KernelInfo kernel_info;
    status = load_kernel(ImageHandle, &kernel_info);
    if (EFI_ERROR(status)) {
        Print(L"Failed to load kernel\n");
        return status;
    }
    
    // Exit boot services
    status = uefi_call_wrapper(BS->ExitBootServices, 2, ImageHandle, map_key);
    if (EFI_ERROR(status)) {
        Print(L"Failed to exit boot services\n");
        return status;
    }
    
    // Jump to kernel - passes 4 arguments as kernel expects
    void (*kernel_entry)(void*, void*, UINT64, UINT64) = (void*)kernel_info.entry_point;
    kernel_entry(gop, memory_map, memory_map_size, descriptor_size);
    
    return EFI_SUCCESS;
}
