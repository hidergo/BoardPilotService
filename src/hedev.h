#ifndef __HEDEV_H
#define __HEDEV_H
#include <stdint.h>
#include <stdlib.h>
#include <hidapi.h>
#include <wchar.h>
#include "cJSON/cJSON.h"

// Device allocation size
#define HED_DEVICE_ALLOC_SIZE   8

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

#endif