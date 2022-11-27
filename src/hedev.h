#ifndef __HEDEV_H
#define __HEDEV_H
#include <stdint.h>
#include <stdlib.h>
#include <hidapi.h>
#include <wchar.h>
#include "cJSON/cJSON.h"

// Device allocation size
#define HED_DEVICE_ALLOC_SIZE   8

// Protocols
// USB
#define HED_PROTO_USB           0
// Bluetooth
#define HED_PROTO_BT            0


// Product definition
struct HEProduct {
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

extern const struct HEProduct PRODUCT_LIST[];

extern const size_t PRODUCT_COUNT;

// Actual devices
struct HEDev {
    struct HEProduct *product;
    wchar_t serial[64];
    uint8_t active;
    // hid path
    char path[256];
    // Protocol
    uint8_t protocol;
};

extern struct HEDev *device_list[HED_DEVICE_ALLOC_SIZE];

// Polls devices
int hedev_poll_usb_devices ();

// Create JSON object from a device
cJSON *hedev_to_json (struct HEDev *device);

// Initializes hedev
void hedev_init ();

// Prints products
void hedev_print_products ();

/**
 * @brief Writes to a device
 * 
 * @param device 
 * @param buffer 
 * @param len 
 * @return Error code (< 0), or bytes written
 */
int device_write (struct HEDev *device, uint8_t *buffer, uint8_t len);

/**
 * @brief Reads from a device. Returns bytes read
 * 
 * @param device 
 * @param buffer 
 * @param len 
 * @return Error code (< 0), or bytes read
 */
int device_read (struct HEDev *device, uint8_t *buffer, uint16_t len);

/**
 * @brief Finds a device. Any of the parameters may be NULL, for example `find_device(NULL, L"ABCDEFG123456789", NULL)`
 * 
 * @param product 
 * @param serial 
 * @param path 
 * @return struct HEDev* 
 */
struct HEDev *find_device (struct HEProduct *product, const wchar_t *serial, const char *path);

#endif