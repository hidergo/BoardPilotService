#ifndef __HEAPI_MSG_H
#define __HEAPI_MSG_H
#include <stdint.h>
#include "hedev.h"

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

#define HIDERGOD_REPORT_SIZE        0x1F
extern uint8_t report_buffer[HIDERGOD_REPORT_SIZE];

// HID MESSAGING

#define HIDERGOD_VALUE_KEY_TIME     0x01

// Package header
PACK(struct hidergod_msg_header {
    // Report ID (always 0x05)
    uint8_t reportId;
    // Command
    uint8_t cmd;
    // Total message size
    uint16_t size;
    // Chunk size (actual message size)
    uint8_t chunkSize;
    // Message chunk offset
    uint16_t chunkOffset;
    // CRC8
    uint8_t crc;
});

#define HIDERGOD_HEADER_SIZE sizeof(struct hidergod_msg_header)

// Set value message
PACK(struct hidergod_msg_set_value {
    // Key
    uint16_t key;
    // Length
    uint8_t length;
    // Data will be at this address
    uint8_t data;
});


// Command
enum hidergod_cmd_t {
    HIDERGOD_CMD_SET_VALUE = 0x01
};

/**
 * @brief Initializes a header. Sets cmd to `cmd` and message size to `size`
 * 
 * @param header 
 * @param cmd 
 * @param size 
 * @return int 
 */
int hedev_build_header (struct hidergod_msg_header *header, enum hidergod_cmd_t cmd, uint16_t size);

/**
 * @brief Sets the device time
 * 
 * @param device 
 * @return int 
 */
int hedev_set_time (struct HEDev *device);



#endif