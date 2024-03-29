#ifndef __BPDEV_H
#define __BPDEV_H
#include <stdint.h>
#include <stdlib.h>
#include <hidapi.h>
#include <wchar.h>
#include "cJSON/cJSON.h"

// Device allocation size
#define BP_DEVICE_ALLOC_SIZE   8

// Protocols
// USB
#define BP_PROTO_USB           0
// Bluetooth
#define BP_PROTO_BT            0


// Product definition
struct BPProduct {
    // Device VID
    uint16_t vendorid;
    // Device PID
    uint16_t productid;

    // Device manufacturer string ex. hid:ergo
    const char *manufacturer;
    // Product string ex. Split(R)
    const char *product;
    // Bluetooth product name match
    const char *product_string;

    // Product revision
    uint8_t rev;
};

extern const struct BPProduct PRODUCT_LIST[];

extern const size_t PRODUCT_COUNT;

// Actual devices
struct BPDev {
    struct BPProduct *product;
    wchar_t serial[64];
    uint8_t active;
    // hid path
    char path[256];
    // Protocol
    uint8_t protocol;
};

extern struct BPDev *device_list[BP_DEVICE_ALLOC_SIZE];

// Polls devices
int bpdev_poll_usb_devices ();

// Create JSON object from a device
cJSON *bpdev_to_json (struct BPDev *device);

// Initializes bpdev
void bpdev_init ();

// Prints products
void bpdev_print_products ();

/**
 * @brief Writes to a device
 * 
 * @param device 
 * @param buffer 
 * @param len 
 * @return Error code (< 0), or bytes written
 */
int device_write (struct BPDev *device, uint8_t *buffer, uint8_t len);

/**
 * @brief Reads from a device. Returns bytes read
 * 
 * @param device 
 * @param buffer 
 * @param len 
 * @return Error code (< 0), or bytes read
 */
int device_read (struct BPDev *device, uint8_t *buffer, uint16_t len);

/**
 * @brief Finds a device. Any of the parameters may be NULL, for example `find_device(NULL, L"ABCDEFG123456789", NULL)`
 * 
 * @param product 
 * @param serial 
 * @param path 
 * @return struct HEDev* 
 */
struct BPDev *find_device (struct BPProduct *product, const wchar_t *serial, const char *path);

#endif