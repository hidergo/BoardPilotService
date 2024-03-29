#include "bpdev.h"
#include <stdio.h>
#include <string.h>
#include "bpapi.h"
#include "zmk_control.h"

// Products list, used to find devices
const struct BPProduct PRODUCT_LIST[] = {
    {
        // hid:ergo Disconnect MK1
        0x1915,     // VID 
        0x520f,     // PID
        "hid:ergo", // Manufacturer
        "Disconnect MK1",    // Product name
        "Disconnect MK1", // Product string for bluetooth
        1           // Revision
    }
    /*,
    {
        // hid:ergo Disconnect MK1 (left)
        0x1915,     // VID 
        0x5210,     // PID
        "hid:ergo", // Manufacturer
        "Disconnect MK1(L)",    // Product name
        "Disconnect MK1",       // Product string for bluetooth (never connected)
        1           // Revision
    }*/
};
const size_t PRODUCT_COUNT = sizeof(PRODUCT_LIST) / sizeof(struct BPProduct);

// Device list TODO: Dynamic allocation later
struct BPDev *device_list[BP_DEVICE_ALLOC_SIZE];
// Live device count
int device_count = 0;

int device_write (struct BPDev *device, uint8_t *buffer, uint8_t len) {
    hid_device *dev = hid_open_path(device->path);
    if(dev == NULL) {
        printf("[WARNING] Could not open device\n");
        return -1;
    }
    memset(report_buffer, 0, sizeof(report_buffer));
    memcpy(report_buffer, buffer, sizeof(report_buffer) < len ? sizeof(report_buffer) : len);

    int err;
#if defined(_WIN32)
    struct hid_device_info *devinfo = hid_get_device_info(dev);
    if(devinfo->bus_type == HID_API_BUS_BLUETOOTH) {
        err = hid_set_output_report(dev, report_buffer, sizeof(report_buffer));
    }
    else {
        err = hid_write(dev, report_buffer, sizeof(report_buffer));
    }
    
#elif defined(__linux__)
    err = hid_write(dev, report_buffer, sizeof(report_buffer));
#endif
    if(err < 0) {
        printf("[ERROR] Failed to write to device: %ls\n", hid_error(dev));
    }
    else {
        //printf("[DEBUG] Wrote to %s\n", device->path);
    }

    hid_close(dev);
    return err;
}

int device_read (struct BPDev *device, uint8_t *buffer, uint16_t len) {
    hid_device *dev = hid_open_path(device->path);
    if(dev == NULL) {
        printf("[WARNING] Could not open device\n");
        return -1;
    }
    int err = 0;

    // Set blocking mode
    hid_set_nonblocking(dev, 0);

    // Temp buffer
    uint8_t temp_buffer[ZMK_CONTROL_REPORT_SIZE];

    int rlen = 0;
    struct zmk_control_msg_header *hdr = NULL;
    // Receive message, which is written to buffer
#if defined(_WIN32)
    struct hid_device_info *devinfo = hid_get_device_info(dev);
    if(devinfo->bus_type == HID_API_BUS_BLUETOOTH) {
        // TODO: does hid_get_input_report return length?
        while((err = hid_get_input_report(dev, temp_buffer, ZMK_CONTROL_REPORT_SIZE)) > 0) {
            if(buffer[0] == 0x05) {
                if(rlen + err > len)
                    break;
                // Header received
                hdr = (struct zmk_control_msg_header*)temp_buffer;
                memcpy(&buffer[hdr->chunk_offset], temp_buffer + sizeof(struct zmk_control_msg_header), hdr->chunk_size);
                rlen += err;
            }
        }
    }
    else {
        while((err = hid_read_timeout(dev, temp_buffer, ZMK_CONTROL_REPORT_SIZE, 100)) > 0) {
            if(temp_buffer[0] == 0x05) {
                if(rlen + err > len)
                    break;
                // Header received
                hdr = (struct zmk_control_msg_header*)temp_buffer;
                memcpy(&buffer[hdr->chunk_offset], temp_buffer + sizeof(struct zmk_control_msg_header), hdr->chunk_size);
                rlen += err;
            }
        }
    }
#elif defined(__linux__)
    while((err = hid_read_timeout(dev, temp_buffer, ZMK_CONTROL_REPORT_SIZE, 100)) > 0) {
        if(temp_buffer[0] == 0x05) {
            if(rlen + err > len)
                break;
            // Header received
            hdr = (struct zmk_control_msg_header*)temp_buffer;
            memcpy(&buffer[hdr->chunk_offset], temp_buffer + sizeof(struct zmk_control_msg_header), hdr->chunk_size);
            rlen += err;
        }
    }
#endif
    if(err < 0) {
        rlen = err;
        printf("[ERROR] Failed to read from device: %ls\n", hid_error(dev));
    }
    else {
        printf("[DEBUG] read from %s l:%i (data:%i)\n", device->path, rlen, hdr == NULL ? 0 : hdr->size);
    }
    hid_close(dev);
    if(hdr != NULL) {
        return hdr->size;
    }

    return 0;
}

int add_device (struct BPProduct *product, struct hid_device_info *info) {
    int f = 0;
    for(int i = 0; i < BP_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] == NULL) {
            struct BPDev *dev = malloc(sizeof(struct BPDev));
            dev->product = product;
            #if defined(_WIN32)
            strncpy_s(dev->path, 256, info->path, 255);
            wcsncpy_s(dev->serial, 64, info->serial_number, 63);
            #else
            strncpy(dev->path, info->path, 255);
            wcsncpy(dev->serial, info->serial_number, 63);
            #endif
            dev->active = 1;
            if(dev->serial[0] == 0) {
                // Bluetooth device
                dev->protocol = BP_PROTO_BT;
            }
            else {
                // USB device
                dev->protocol = BP_PROTO_USB;
            }
            device_list[i] = dev;
            f = 1;
            bpapi_trigger_event(APIEVENT_DEVICE_CONNECTED, dev);
            printf("Device %s %s (%X:%X) connected\n", dev->product->manufacturer, dev->product->product, dev->product->vendorid, dev->product->productid);
            // Set device time
            zmk_control_msg_set_time(dev);
            
            break;
        }
    }

    if(!f) {
        // Too many devices
        printf("[ERROR] Too many devices\n");
        return 1;
    }

    device_count++;
    return 0;
}

struct BPDev *find_device (struct BPProduct *product, const wchar_t *serial, const char *path) {
    for(int i = 0; i < BP_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] != NULL) {
            if(product == NULL || product == device_list[i]->product) {
                if(serial == NULL) {
                    // Check hid path
                    if(strcmp(path, device_list[i]->path) == 0) {
                        return device_list[i];
                    }
                }
                else {
                    // Check serial
                    if(wcscmp(serial, device_list[i]->serial) == 0) {
                        return device_list[i];
                    }
                }
            }
        }
    }
    return NULL;
}

void remove_device (struct BPDev *dev) {
    for(int i = 0; i < BP_DEVICE_ALLOC_SIZE; i++) {
        if(dev == device_list[i]) {
            bpapi_trigger_event(APIEVENT_DEVICE_DISCONNECTED, dev);
            
            printf("Device %s %s (%X:%X) disconnected\n", dev->product->manufacturer, dev->product->product, dev->product->vendorid, dev->product->productid);
            free(dev);
            device_list[i] = NULL;
            device_count--;
            return;
        }
    }
}

// Removes inactive devices
void device_cleanup () {
    for(int i = 0; i < BP_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] != NULL && !device_list[i]->active) {
            remove_device(device_list[i]);
        }
    }
}

int bpdev_poll_usb_devices () {
    // Reset device active flags
    for(int i = 0; i < BP_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] != NULL) {
            device_list[i]->active = 0;
        }
    }
    // Need to search for all devices because bluetooth devices don't have VID/PID
    struct hid_device_info *devs = hid_enumerate(0x00, 0x00); 

    struct hid_device_info *curdev = devs;
    char buff[64];
    while (curdev != NULL) {

        for(int i = 0; i < (int)PRODUCT_COUNT; i++) {
            // Match to a device
            struct BPProduct *prod = (struct BPProduct *)&PRODUCT_LIST[i];
            uint8_t dev_ok = 0;

            // USB device - match product/vendor ID
            if((uint16_t)curdev->product_id == prod->productid && (uint16_t)curdev->vendor_id == prod->vendorid) {
                dev_ok = 1;
            }
            else {
                // Might be bluetooth device - match product/manufacturer name
                snprintf(buff, 63, "%ls", curdev->product_string);
                if(strcmp(buff, prod->product_string) == 0) {
                    dev_ok = 1;
                }
            }

            // Device not ok
            if(!dev_ok)
                continue;

            // TODO: how to handle multiple devices without serial number?
            struct BPDev *dev = find_device(prod, curdev->serial_number, curdev->path);
            if(dev == NULL) {
                // Fix for Windows not allowing to read/write from/to a certain input device
                int dev_open_read = 0;
                hid_device *hiddev = hid_open_path(curdev->path);
                if(hiddev != NULL) {
                    struct zmk_control_msg_header open_header;
                    zmk_control_build_header(&open_header, ZMK_CONTROL_CMD_CONNECT, 0x00);
                    memset(report_buffer, 0, sizeof(report_buffer));
                    memcpy(report_buffer, &open_header, sizeof(struct zmk_control_msg_header));

                #if defined(_WIN32)
                    if(curdev->bus_type == HID_API_BUS_BLUETOOTH) {
                        if(hid_set_output_report(hiddev, report_buffer, sizeof(report_buffer)) >= 0) {
                             dev_open_read = 1;
                        }
                    }
                    else {
                        if(hid_write(hiddev, report_buffer, sizeof(report_buffer)) >= 0) {
                            dev_open_read = 1;
                        }
                    }
                #elif defined(__linux__)
                    if(hid_write(hiddev, report_buffer, sizeof(report_buffer)) >= 0) {
                        dev_open_read = 1;
                    }
                #endif
                    hid_close(hiddev);
                }
                else {
                    //printf("Could not open device");
                }
                if(dev_open_read) {
                    // Add new device
                    add_device(prod, curdev);
                }
            }
            else {
                // Update device active flag
                dev->active = 1;
            }
        }
        
        curdev = curdev->next;
    }
    hid_free_enumeration(devs);
    // Remove inactive devices
    device_cleanup();

    return 0;
}

cJSON *bpdev_to_json (struct BPDev *device) {
    cJSON *jsn = cJSON_CreateObject();
    // PRODUCT INFO
    cJSON *prod = cJSON_AddObjectToObject(jsn, "product");
    cJSON_AddNumberToObject(prod, "vid", device->product->vendorid);
    cJSON_AddNumberToObject(prod, "pid", device->product->productid);
    cJSON_AddStringToObject(prod, "manufacturer", device->product->manufacturer);
    cJSON_AddStringToObject(prod, "product", device->product->product);
    cJSON_AddNumberToObject(prod, "rev", device->product->rev);

    // DEVICE INFO
    char buff[64];
    snprintf(buff, 63, "%ls", device->serial);
    cJSON *dev = cJSON_AddObjectToObject(jsn, "device");
    cJSON_AddStringToObject(dev, "serial", buff);
    cJSON_AddStringToObject(dev, "protocol", device->protocol == BP_PROTO_USB ? "usb" : "bt");

    return jsn;
}

void bpdev_init () {
    memset(device_list, 0, sizeof(device_list));
}

void bpdev_print_products () {
    printf("Devices:\n");
    for(int i = 0; i < (int)PRODUCT_COUNT; i++) {
        const struct BPProduct *dev = &PRODUCT_LIST[i];
        printf("  Device %i:\n", i);
        printf("    VID:PID:      %04X:%04X\n", dev->vendorid, dev->productid);
        printf("    Manufacturer: %s\n", dev->manufacturer);
        printf("    Product:      %s\n", dev->product);
    }
}