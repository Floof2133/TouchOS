// drivers/usb/xhci.c
#include "xhci.h"
#include "pci.h"

typedef struct {
    volatile uint32_t* op_regs;     // Operational registers
    volatile uint32_t* runtime_regs; // Runtime registers
    volatile uint32_t* doorbell;     // Doorbell array
    
    // Data structures
    uint64_t* dcbaa;      // Device Context Base Address Array
    xhci_trb_t* cmd_ring; // Command ring
    xhci_trb_t* event_ring; // Event ring
    
    // State tracking
    uint32_t cmd_ring_enqueue;
    uint32_t event_ring_dequeue;
    spinlock_t lock;
} xhci_controller_t;

void xhci_init(pci_device_t* device) {
    xhci_controller_t* xhci = kmalloc(sizeof(xhci_controller_t));
    
    // Map MMIO registers
    uint64_t bar0 = pci_read_bar(device, 0);
    xhci->op_regs = (uint32_t*)vmm_map_mmio(bar0, 0x1000);
    
    // Get capability registers
    uint32_t* cap_regs = (uint32_t*)xhci->op_regs;
    uint32_t caplength = cap_regs[0] & 0xFF;
    uint32_t hcsparams1 = cap_regs[1];
    
    // Calculate register offsets
    xhci->op_regs = (uint32_t*)((uint8_t*)cap_regs + caplength);
    uint32_t rtsoff = (cap_regs[4] >> 16) & 0xFFFF;
    xhci->runtime_regs = (uint32_t*)((uint8_t*)cap_regs + rtsoff);
    uint32_t dboff = cap_regs[5];
    xhci->doorbell = (uint32_t*)((uint8_t*)cap_regs + dboff);
    
    // Reset controller
    xhci->op_regs[XHCI_USBCMD] |= USBCMD_RESET;
    while (xhci->op_regs[XHCI_USBCMD] & USBCMD_RESET);
    
    // Wait for controller ready
    while (xhci->op_regs[XHCI_USBSTS] & USBSTS_CNR);
    
    // Set up data structures
    uint32_t max_slots = (hcsparams1 >> 0) & 0xFF;
    
    // Device Context Base Address Array
    xhci->dcbaa = (uint64_t*)alloc_pages(1);
    memset(xhci->dcbaa, 0, 4096);
    xhci->op_regs[XHCI_DCBAAP_LO] = (uint32_t)(uintptr_t)xhci->dcbaa;
    xhci->op_regs[XHCI_DCBAAP_HI] = (uint32_t)((uintptr_t)xhci->dcbaa >> 32);
    
    // Command ring
    xhci->cmd_ring = (xhci_trb_t*)alloc_pages(1);
    memset(xhci->cmd_ring, 0, 4096);
    xhci->cmd_ring[255].control = TRB_TYPE(TRB_LINK) | TRB_TC;
    xhci->cmd_ring[255].parameter = (uint64_t)xhci->cmd_ring;
    
    uint64_t cmd_ring_addr = (uint64_t)xhci->cmd_ring | 1;  // Set RCS bit
    xhci->op_regs[XHCI_CRCR_LO] = (uint32_t)cmd_ring_addr;
    xhci->op_regs[XHCI_CRCR_HI] = (uint32_t)(cmd_ring_addr >> 32);
    
    // Event ring segment table
    xhci_erst_entry_t* erst = (xhci_erst_entry_t*)alloc_pages(1);
    xhci->event_ring = (xhci_trb_t*)alloc_pages(1);
    memset(xhci->event_ring, 0, 4096);
    
    erst[0].ring_segment_base = (uint64_t)xhci->event_ring;
    erst[0].ring_segment_size = 256;
    
    // Set up interrupter 0
    xhci->runtime_regs[XHCI_IMAN(0)] = 0x2;  // Clear pending
    xhci->runtime_regs[XHCI_IMOD(0)] = 4000;  // 1ms moderation
    xhci->runtime_regs[XHCI_ERSTSZ(0)] = 1;
    xhci->runtime_regs[XHCI_ERSTBA_LO(0)] = (uint32_t)(uintptr_t)erst;
    xhci->runtime_regs[XHCI_ERSTBA_HI(0)] = (uint32_t)((uintptr_t)erst >> 32);
    xhci->runtime_regs[XHCI_ERDP_LO(0)] = (uint32_t)(uintptr_t)xhci->event_ring;
    xhci->runtime_regs[XHCI_ERDP_HI(0)] = (uint32_t)((uintptr_t)xhci->event_ring >> 32);
    
    // Configure controller
    xhci->op_regs[XHCI_CONFIG] = max_slots;
    
    // Start controller
    xhci->op_regs[XHCI_USBCMD] |= USBCMD_RUN | USBCMD_INTE;
    
    // Register interrupt handler
    register_interrupt_handler(device->irq, xhci_interrupt_handler, xhci);
    
    // Enable interrupts
    xhci->runtime_regs[XHCI_IMAN(0)] |= 0x2;
    
    // Enumerate ports
    uint32_t portsc_base = 0x400;
    uint32_t max_ports = (hcsparams1 >> 24) & 0xFF;
    
    for (int i = 0; i < max_ports; i++) {
        uint32_t portsc = xhci->op_regs[(portsc_base + i * 0x10) / 4];
        if (portsc & PORTSC_CCS) {
            xhci_port_reset(xhci, i);
            xhci_enumerate_device(xhci, i);
        }
    }
}

void xhci_enumerate_device(xhci_controller_t* xhci, int port) {
    // Enable slot
    xhci_trb_t enable_slot = {
        .control = TRB_TYPE(TRB_ENABLE_SLOT) | TRB_CYCLE
    };
    
    uint32_t slot_id = xhci_send_command(xhci, &enable_slot);
    
    // Allocate device context
    xhci_device_context_t* dev_ctx = (xhci_device_context_t*)alloc_pages(1);
    memset(dev_ctx, 0, sizeof(xhci_device_context_t));
    xhci->dcbaa[slot_id] = (uint64_t)dev_ctx;
    
    // Allocate input context
    xhci_input_context_t* input_ctx = (xhci_input_context_t*)alloc_pages(2);
    memset(input_ctx, 0, sizeof(xhci_input_context_t));
    
    // Configure slot context
    input_ctx->control.add_flags = 0x3;  // Add slot and EP0
    input_ctx->slot.root_hub_port = port + 1;
    input_ctx->slot.route_string = 0;
    input_ctx->slot.context_entries = 1;
    input_ctx->slot.speed = xhci_get_port_speed(xhci, port);
    
    // Configure endpoint 0
    xhci_trb_t* ep0_ring = (xhci_trb_t*)alloc_pages(1);
    memset(ep0_ring, 0, 4096);
    
    input_ctx->endpoints[0].tr_dequeue_ptr = (uint64_t)ep0_ring | 1;
    input_ctx->endpoints[0].ep_type = EP_TYPE_CONTROL;
    input_ctx->endpoints[0].max_packet_size = 64;  // Default for FS/LS
    input_ctx->endpoints[0].error_count = 3;
    
    // Address device
    xhci_trb_t addr_device = {
        .parameter = (uint64_t)input_ctx,
        .control = TRB_TYPE(TRB_ADDRESS_DEVICE) | (slot_id << 24) | TRB_CYCLE
    };
    
    xhci_send_command(xhci, &addr_device);
    
    // Get device descriptor
    usb_device_descriptor_t desc;
    xhci_control_transfer(xhci, slot_id, 
                         USB_REQ_GET_DESCRIPTOR | USB_DIR_IN,
                         USB_DT_DEVICE << 8, 0, 
                         &desc, sizeof(desc));
    
    // Check if it's a HID device (potential touchscreen)
    if (desc.bDeviceClass == USB_CLASS_HID || 
        (desc.bDeviceClass == 0 && desc.idVendor == ACER_VENDOR_ID)) {
        // Configure HID interface
        xhci_configure_hid_device(xhci, slot_id, &desc);
    }
}
// This is Rev1 Of the USB drivers, subject to change, because of bugs...
// THANK YOU XansiVA FOR HELPING OUT!!, Especially bug fixes.
