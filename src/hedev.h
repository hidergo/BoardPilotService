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

// HID MESSAGING

#define HIDERGOD_VALUE_KEY_TIME     0x01

// Package header
struct __attribute__((packed)) hidergod_msg_header {
    // Report ID (always 0)
    uint8_t reportId;
    // Command
    uint8_t cmd;
    // Total message size
    uint16_t size;
    // Chunk size (actual message size)
    uint8_t chunkSize;
    // Message chunk offset
    uint16_t chunkOffset;
    // CRC16
    uint16_t crc;
};

struct __attribute__((packed)) hidergod_msg_set_value {
    // Key
    uint16_t key;
    // Length
    uint8_t length;
    // Data will be at this address
    uint8_t data;
};


// Command
enum hidergod_cmd_t {
    HIDERGOD_CMD_SET_VALUE = 0x01
};

#endif