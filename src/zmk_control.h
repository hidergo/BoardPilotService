#ifndef __ZMK_CONTROL_H
#define __ZMK_CONTROL_H

#include <stdint.h>
#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

#include "hedef.h"

// Maximum report size including 1 byte of report ID
#define ZMK_CONTROL_REPORT_SIZE     0x20

extern uint8_t report_buffer[ZMK_CONTROL_REPORT_SIZE];

// Commands
enum zmk_control_cmd_t {
    // Invalid/reserved command
    ZMK_CONTROL_CMD_INVALID =       0x00,
    // Connection checking command
    ZMK_CONTROL_CMD_CONNECT =       0x01,

    // Sets a configuration value
    ZMK_CONTROL_CMD_SET_CONFIG =    0x11,
    // Gets a configuration value
    ZMK_CONTROL_CMD_GET_CONFIG =    0x12
    
};

// Package header
// TODO: Do the messages need header on every chunk? large overhead but safer transfer...
PACK(struct zmk_control_msg_header {
    // Report ID (should always be 0x05)
    uint8_t report_id; // Not received for some reason?
    // Command (enum zmk_control_cmd_t)
    uint8_t cmd;
    // Total message size
    uint16_t size;
    // Chunk size (actual message size)
    uint8_t chunk_size;
    // Message chunk offset
    uint16_t chunk_offset;
    // CRC8
    uint8_t crc;
});

// Size of the message without header
#define ZMK_CONTROL_REPORT_DATA_SIZE (ZMK_CONTROL_REPORT_SIZE - sizeof(struct zmk_control_msg_header))

// Set config message structure
PACK(struct zmk_control_msg_set_config {
    // Config key. config.h/enum zmk_config_key
    uint16_t key;
    // Config size
    uint16_t size;
    // Is the data to be saved to NVS
    uint8_t save;
    // Data. Represented as u8 but will be allocated to contain variable of length "size"
    uint8_t data;
});

// Get config message structure
PACK(struct zmk_control_msg_get_config {
    // Config key. config.h/enum zmk_config_key
    uint16_t key;
    // Config size
    uint16_t size;
    // Data. Represented as u8 but will be allocated to contain variable of length "size"
    uint8_t data;
});


/**
 * @brief Configuration keys, casted as uint16_t
 * 
 * More fields to be added
 */
enum zmk_config_key {
    // Invalid key
    ZMK_CONFIG_KEY_INVALID =                0x0000,
    // --------------------------------------------------------------
    // 0x0001 - 0x3FFF: (Recommended) saveable fields
    // Fields that should be saved to NVS, such as keymap or mouse sensitivity
    // --------------------------------------------------------------

    // 0x0001 - 0x001F: Misc device config fields
    // Device name for BLE/USB
    ZMK_CONFIG_KEY_DEVICE_NAME =            0x0001,


    // 0x0020 - 0x003F: Keyboard configurations 
    // Keymap
    ZMK_CONFIG_KEY_KEYMAP =                 0x0020,

    // 0x0040 - 0x005F: Mouse/trackpad configurations
    // Mouse sensitivity
    ZMK_CONFIG_KEY_MOUSE_SENSITIVITY =      0x0040,
    // Mouse Y scroll sensitivity
    ZMK_CONFIG_KEY_SCROLL_SENSITIVITY =     0x0041,
    // Mouse X pan sensitivity
    ZMK_CONFIG_KEY_PAN_SENSITIVITY =        0x0042,

    // --------------------------------------------------------------
    // 0x4000 - 0x7FFF: (Recommended) Non-saved fields
    // Fields that do not require saving to NVS, such as time or date
    // --------------------------------------------------------------

    // (int32_t[2]) [0] Unix timestamp of time, [1] timezone in seconds
    ZMK_CONFIG_KEY_DATETIME =               0x4000,


    // --------------------------------------------------------------
    // 0x8000 - 0xFFFF: Custom fields
    // Fields that should be used if custom fields are needed
    // --------------------------------------------------------------
    // hid:ergo device specific fields
    // IQS5XX register configuration
    ZMK_CONFIG_CUSTOM_IQS5XX_REGS =         0x8001,

};

int zmk_control_build_header (struct zmk_control_msg_header *header, enum zmk_control_cmd_t cmd, uint16_t size);
int zmk_control_write_message (struct HEDev *device, struct zmk_control_msg_header *header, uint8_t *data);

/**
 * @brief Write device config field
 * 
 * @param device 
 * @param key 
 * @param data 
 * @param len 
 * @param save 
 * @return int 
 */
int zmk_control_set_config (struct HEDev *device, uint16_t key, void *data, uint16_t len, uint8_t save);

/**
 * @brief Sets the device time
 * 
 * @param device 
 * @return int 
 */
int zmk_control_msg_set_time (struct HEDev *device);
/**
 * @brief Sets mouse sensitivity
 * 
 * @param device 
 * @param sensitivity Value range multiplier, calculated as (float)sensitivity / 128.0F, so 128 is 1
 * @return int 
 */
int zmk_control_msg_set_mouse_sensitivity (struct HEDev *device, uint8_t sensitivity);

/**
 * @brief Sets the iqs5xx registers
 * 
 * @param device 
 * @param config 
 * @param save 
 * @return int 
 */
int zmk_control_msg_set_iqs5xx_registers (struct HEDev *device, struct iqs5xx_reg_config config, uint8_t save);

#endif