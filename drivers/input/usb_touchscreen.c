// drivers/input/usb_touchscreen.c
#include "usb.h"
#include "hid.h"
#include "input.h"

typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t tip_switch;
    uint8_t contact_id;
} touch_point_t;

typedef struct {
    usb_device_t* device;
    uint8_t interface;
    uint8_t endpoint;
    
    // HID report descriptor parsing
    uint16_t max_x;
    uint16_t max_y;
    uint8_t max_contacts;
    
    // Current state
    touch_point_t contacts[10];
    spinlock_t lock;
    
    // Calibration for Acer T230H
    uint32_t cal_x_min;
    uint32_t cal_x_max;
    uint32_t cal_y_min;
    uint32_t cal_y_max;
} usb_touchscreen_t;

void usb_touchscreen_probe(usb_device_t* device, uint8_t interface) {
    usb_touchscreen_t* ts = kmalloc(sizeof(usb_touchscreen_t));
    memset(ts, 0, sizeof(usb_touchscreen_t));
    
    ts->device = device;
    ts->interface = interface;
    
    // Get HID report descriptor
    uint8_t report_desc[256];
    int desc_len = usb_control_transfer(device,
                                       USB_REQ_GET_DESCRIPTOR | USB_DIR_IN | USB_TYPE_STANDARD,
                                       (HID_DT_REPORT << 8), interface,
                                       report_desc, sizeof(report_desc));
    
    // Parse report descriptor for touchscreen capabilities
    hid_parse_touchscreen_descriptor(report_desc, desc_len, ts);
    
    // Acer T230H specific initialization
    if (device->vendor_id == 0x0408 && device->product_id == 0x3000) {
        ts->max_x = 4096;
        ts->max_y = 4096;
        ts->max_contacts = 2;
        
        // Calibration values for 1920x1080
        ts->cal_x_min = 150;
        ts->cal_x_max = 3946;
        ts->cal_y_min = 130;
	ts->cal_y_max = 3966;
        
        // Send vendor-specific initialization for Acer T230H
        uint8_t init_cmd[] = {0x02, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
        usb_control_transfer(device,
                           USB_REQ_SET_CONFIGURATION | USB_DIR_OUT | USB_TYPE_CLASS,
                           0x0301, interface, init_cmd, sizeof(init_cmd));
    }
    
    // Find interrupt IN endpoint
    usb_interface_descriptor_t* iface_desc = usb_get_interface_descriptor(device, interface);
    for (int i = 0; i < iface_desc->bNumEndpoints; i++) {
        usb_endpoint_descriptor_t* ep = usb_get_endpoint_descriptor(device, interface, i);
        if ((ep->bEndpointAddress & 0x80) && 
            (ep->bmAttributes & 0x03) == USB_ENDPOINT_INTERRUPT) {
            ts->endpoint = ep->bEndpointAddress;
            
            // Start interrupt transfers
            usb_interrupt_transfer(device, ts->endpoint, 
                                 touchscreen_interrupt_handler, ts,
                                 ep->wMaxPacketSize, ep->bInterval);
            break;
        }
    }
    
    // Register with input subsystem
    input_device_t* input = input_allocate_device();
    input->name = "Acer T230H Touchscreen";
    input->type = INPUT_TYPE_TOUCHSCREEN;
    input->capabilities = INPUT_CAP_MT | INPUT_CAP_ABS;
    input->max_x = 1920;
    input->max_y = 1080;
    input->max_contacts = ts->max_contacts;
    input->private_data = ts;
    
    input_register_device(input);
}

void touchscreen_interrupt_handler(void* data, uint8_t* buffer, int length) {
    usb_touchscreen_t* ts = (usb_touchscreen_t*)data;
    
    spin_lock(&ts->lock);
    
    // Parse HID report based on report ID
    uint8_t report_id = buffer[0];
    
    if (report_id == 0x01) {  // Single touch report
        uint16_t x = (buffer[2] << 8) | buffer[1];
        uint16_t y = (buffer[4] << 8) | buffer[3];
        uint8_t tip = buffer[5] & 0x01;
        
        // Apply calibration
        x = touchscreen_calibrate_x(ts, x);
        y = touchscreen_calibrate_y(ts, y);
        
        // Send to input system
        input_event_t event = {
            .type = EV_ABS,
            .code = ABS_X,
            .value = x
        };
        input_report_event(&event);
        
        event.code = ABS_Y;
        event.value = y;
        input_report_event(&event);
        
        event.type = EV_KEY;
        event.code = BTN_TOUCH;
        event.value = tip;
        input_report_event(&event);
        
        input_sync();
        
    } else if (report_id == 0x02) {  // Multitouch report
        int contact_count = buffer[1];
        
        for (int i = 0; i < contact_count && i < ts->max_contacts; i++) {
            int offset = 2 + (i * 6);
            uint8_t contact_id = buffer[offset];
            uint8_t tip = buffer[offset + 1] & 0x01;
            uint16_t x = (buffer[offset + 3] << 8) | buffer[offset + 2];
            uint16_t y = (buffer[offset + 5] << 8) | buffer[offset + 4];
            
            // Apply calibration
            x = touchscreen_calibrate_x(ts, x);
            y = touchscreen_calibrate_y(ts, y);
            
            // Update contact tracking
            ts->contacts[contact_id].x = x;
            ts->contacts[contact_id].y = y;
            ts->contacts[contact_id].tip_switch = tip;
            
            // Send MT events
            input_event_t event = {
                .type = EV_ABS,
                .code = ABS_MT_SLOT,
                .value = contact_id
            };
            input_report_event(&event);
            
            if (tip) {
                event.code = ABS_MT_TRACKING_ID;
                event.value = contact_id;
                input_report_event(&event);
                
                event.code = ABS_MT_POSITION_X;
                event.value = x;
                input_report_event(&event);
                
                event.code = ABS_MT_POSITION_Y;
                event.value = y;
                input_report_event(&event);
            } else {
                event.code = ABS_MT_TRACKING_ID;
                event.value = -1;  // Release
                input_report_event(&event);
            }
        }
        
        input_sync();
    }
    
    spin_unlock(&ts->lock);
    
    // Resubmit interrupt transfer
    usb_interrupt_transfer(ts->device, ts->endpoint,
                         touchscreen_interrupt_handler, ts,
                         64, 10);
}

uint16_t touchscreen_calibrate_x(usb_touchscreen_t* ts, uint16_t raw_x) {
    if (raw_x < ts->cal_x_min) raw_x = ts->cal_x_min;
    if (raw_x > ts->cal_x_max) raw_x = ts->cal_x_max;
    
    return ((raw_x - ts->cal_x_min) * 1920) / (ts->cal_x_max - ts->cal_x_min);
}

uint16_t touchscreen_calibrate_y(usb_touchscreen_t* ts, uint16_t raw_y) {
    if (raw_y < ts->cal_y_min) raw_y = ts->cal_y_min;
    if (raw_y > ts->cal_y_max) raw_y = ts->cal_y_max;
    
    return ((raw_y - ts->cal_y_min) * 1080) / (ts->cal_y_max - ts->cal_y_min);
}
// USB Touch input Rev1
// If you have a display that is not supported, feel free to add it and either fork or send a pull request

