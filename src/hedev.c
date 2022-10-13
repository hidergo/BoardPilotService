#include "hedev.h"
#include <stdio.h>
#include <string.h>
#include "heapi.h"
#include "hedev_msg.h"

// hid:ergo products, used to find devices
const struct HEProduct PRODUCT_LIST[] = {
    {
        0x1915,     // VID 
        0x520f,     // PID
        "hid:ergo", // Manufacturer
        "split",    // Product name
        "hid:ergo split", // Product string for bluetooth
        1           // Revision
    }
};
const size_t PRODUCT_COUNT = sizeof(PRODUCT_LIST) / sizeof(struct HEProduct);

// Device list TODO: Dynamic allocation later
struct HEDev *device_list[HED_DEVICE_ALLOC_SIZE];
// Live device count
int device_count = 0;


int device_write (struct HEDev *device, uint8_t *buffer, uint8_t len) {
    hid_device *dev = hid_open_path(device->path);
    if(dev == NULL) {
        printf("[WARNING] Could not open device\n");
        return -1;
    }
    memset(report_buffer, 0, sizeof(report_buffer));
    memcpy(report_buffer, buffer, len);
    int err = hid_write(dev, report_buffer, sizeof(report_buffer));
    if(err == -1) {
        printf("[ERROR] Failed to write to device: %ls\n", hid_error(dev));
    }
    else {
        printf("[DEBUG] Wrote to %s\n", device->path);
    }

    hid_close(dev);
    return 0;
}

int add_device (struct HEProduct *product, struct hid_device_info *info) {
    int f = 0;
    for(int i = 0; i < HED_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] == NULL) {
            struct HEDev *dev = malloc(sizeof(struct HEDev));
            dev->product = product;
            strncpy(dev->path, info->path, 255);
            dev->active = 1;
            wcsncpy(dev->serial, info->serial_number, 63);
            if(dev->serial[0] == 0) {
                // Bluetooth device
                dev->protocol = HED_PROTO_BT;
            }
            else {
                // USB device
                dev->protocol = HED_PROTO_USB;
            }
            device_list[i] = dev;
            f = 1;
            heapi_trigger_event(APIEVENT_DEVICE_CONNECTED, dev);
            printf("Device %s %s (%X:%X) connected\n", dev->product->manufacturer, dev->product->product, dev->product->vendorid, dev->product->productid);
            // Set device time
            hedev_set_time(dev);
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

struct HEDev *find_device (struct HEProduct *product, const wchar_t *serial, const char *path) {
    for(int i = 0; i < HED_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] != NULL) {
            if(product == device_list[i]->product) {
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

void remove_device (struct HEDev *dev) {
    for(int i = 0; i < HED_DEVICE_ALLOC_SIZE; i++) {
        if(dev == device_list[i]) {
            heapi_trigger_event(APIEVENT_DEVICE_DISCONNECTED, dev);
            
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
    for(int i = 0; i < HED_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] != NULL && !device_list[i]->active) {
            remove_device(device_list[i]);
        }
    }
}

int hedev_poll_usb_devices () {
    // Reset device active flags
    for(int i = 0; i < HED_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] != NULL) {
            device_list[i]->active = 0;
        }
    }
    // Need to search for all devices because bluetooth devices don't have VID/PID
    struct hid_device_info *devs = hid_enumerate(0x00, 0x00); 

    struct hid_device_info *curdev = devs;
    char buff[64];
    while (curdev != NULL) {

        for(int i = 0; i < PRODUCT_COUNT; i++) {
            // Match to a device
            struct HEProduct *prod = (struct HEProduct *)&PRODUCT_LIST[i];
            uint8_t dev_ok = 0;
            if(curdev->product_id == 0x0000 && curdev->vendor_id == 0x0000) {
                // Bluetooth device - match product/manufacturer name
                snprintf(buff, 63, "%ls", curdev->product_string);
                if(strcmp(buff, prod->product_string) == 0) {
                    dev_ok = 1;
                }
            }
            else {
                // USB device - match product/vendor ID
                if((uint16_t)curdev->product_id == prod->productid && (uint16_t)curdev->vendor_id == prod->vendorid) {
                    dev_ok = 1;
                }
            }
            // Device not ok
            if(!dev_ok)
                continue;

            // TODO: how to handle multiple devices without serial number?
            struct HEDev *dev = find_device(prod, curdev->serial_number, curdev->path);
            if(dev == NULL) {
                // Fix for Windows not allowing to read/write from/to a certain input device
                int dev_open_read = 0;
                hid_device *hiddev = hid_open_path(curdev->path);
                if(hiddev != NULL) {
                    hid_set_nonblocking(hiddev, 1);
                    uint8_t buff[2] = {0x05, 0x00};
                    memset(report_buffer, 0, sizeof(report_buffer));
                    report_buffer[0] = 0x05;
                    report_buffer[0] = 0x00;

                    if(hid_write(hiddev, report_buffer, sizeof(report_buffer)) >= 0) {
                        dev_open_read = 1;
                    }
                    hid_set_nonblocking(hiddev, 0);
                    hid_close(hiddev);
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

cJSON *hedev_to_json (struct HEDev *device) {
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
    cJSON_AddStringToObject(dev, "protocol", device->protocol == HED_PROTO_USB ? "usb" : "bt");

    return jsn;
}

void hedev_init () {
    memset(device_list, 0, sizeof(device_list));
}

void hedev_print_products () {
    printf("Devices:\n");
    for(int i = 0; i < PRODUCT_COUNT; i++) {
        const struct HEProduct *dev = &PRODUCT_LIST[i];
        printf("  Device %i:\n", i);
        printf("    VID:PID:      %04X:%04X\n", dev->vendorid, dev->productid);
        printf("    Manufacturer: %s\n", dev->manufacturer);
        printf("    Product:      %s\n", dev->product);
    }
}