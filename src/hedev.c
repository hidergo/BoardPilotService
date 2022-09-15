#include "hedev.h"
#include <stdio.h>
#include <string.h>
#include "heapi.h"

// hid:ergo products, used to find devices
const struct HEProduct PRODUCT_LIST[] = {
    {
        0x1915,     // VID 
        0x520f,     // PID
        "hid:ergo", // Manufacturer
        "Split(R)", // Product name
        1
    }
};
const size_t PRODUCT_COUNT = sizeof(PRODUCT_LIST) / sizeof(struct HEProduct);

// Device list TODO: Dynamic allocation later
struct HEDev *device_list[HED_DEVICE_ALLOC_SIZE];
// Live device count
int device_count = 0;

int add_device (struct HEProduct *product, struct hid_device_info *info) {
    int f = 0;
    for(int i = 0; i < HED_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] == NULL) {
            struct HEDev *dev = malloc(sizeof(struct HEDev));
            dev->product = product;
            dev->active = 1;
            wcsncpy(dev->serial, info->serial_number, 63);
            device_list[i] = dev;
            f = 1;
            heapi_trigger_event(APIEVENT_DEVICE_CONNECTED, dev);
            printf("Device %s %s (%X:%X) connected\n", dev->product->manufacturer, dev->product->product, dev->product->vendorid, dev->product->productid);
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

struct HEDev *find_device (struct HEProduct *product, const wchar_t *serial) {
    for(int i = 0; i < HED_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] != NULL) {
            if(product == device_list[i]->product && wcscmp(serial, device_list[i]->serial) == 0) {
                return device_list[i];
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
    for(int i = 0; i < PRODUCT_COUNT; i++) {
        struct HEProduct *prod = (struct HEProduct *)&PRODUCT_LIST[i];
        struct hid_device_info *devs = hid_enumerate(prod->vendorid, prod->productid);

        struct hid_device_info *curdev = devs;
        while (curdev != NULL) {
            // Compare manufacturer/product name?
            
            struct HEDev *dev = find_device(prod, curdev->serial_number);
            if(dev == NULL) {
                // Add new device
                add_device(prod, curdev);
            }
            else {
                // Update device active flag
                dev->active = 1;
            }
            curdev = curdev->next;
        }

        hid_free_enumeration(devs);
    }
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