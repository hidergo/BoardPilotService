#ifndef __BPDEF_H
#define __BPDEF_H
#include <stdio.h>

#ifdef __GNUC__
#define PACK( __Declaration__ ) __Declaration__ __attribute__((__packed__))
#endif

#ifdef _MSC_VER
#define PACK( __Declaration__ ) __pragma( pack(push, 1) ) __Declaration__ __pragma( pack(pop))
#endif

// hid:ergo custom structs

// Register configuration structure
PACK(struct iqs5xx_reg_config {
    // Refresh rate when the device is active (ms interval)
    uint16_t    activeRefreshRate;
    // Refresh rate when the device is idling (ms interval)
    uint16_t    idleRefreshRate;
    // Which single finger gestures will be enabled
    uint8_t     singleFingerGestureMask;
    // Which multi finger gestures will be enabled
    uint8_t     multiFingerGestureMask;
    // Tap time in ms
    uint16_t    tapTime;
    // Tap distance in pixels
    uint16_t    tapDistance;
    // Touch multiplier
    uint8_t     touchMultiplier;
    // Prox debounce value
    uint8_t     debounce;
    // i2c timeout in ms
    uint8_t     i2cTimeout;
    // Filter settings
    uint8_t     filterSettings;
    uint8_t     filterDynBottomBeta;
    uint8_t     filterDynLowerSpeed;
    uint16_t    filterDynUpperSpeed;

    // Initial scroll distance (px)
    uint16_t    initScrollDistance;
});


PACK(struct zmk_config_keymap_item {
    uint8_t device;
    uint32_t param;
});

#endif