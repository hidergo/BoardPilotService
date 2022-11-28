#include "heapi_msg.h"
#include "hedev.h"
#include "hedef.h"
#include "zmk_control.h"
#include <stdlib.h>
#include <string.h>

// File containing message parsing, response building


int heapi_msg_validate (cJSON *json) {
    cJSON *itm = NULL;
    // Check `cmd`
    itm = cJSON_GetObjectItem(json, "cmd");
    if(itm == NULL || cJSON_IsNumber(json) == cJSON_False) {
        printf("[heapi_msg_validate] Field 'cmd' missing from message\n");
        return 1;
    }
    // Check `reqid`
    itm = cJSON_GetObjectItem(json, "reqid");
    if(itm == NULL || cJSON_IsNumber(json) == cJSON_False) {
        printf("[heapi_msg_validate] Field 'reqid' missing from message\n");
        return 1;
    }

    return 0;
}

int heapi_msg_AUTH (struct HEApiClient *client, cJSON *json, cJSON *resp) {
    int err = 0;
    err = strcmp((const char *)cJSON_GetObjectItem(json, "key")->valuestring, HEAPI_KEY);

    if(!err)
        client->authenticated = 1;

    // RESPONSE
    cJSON_AddBoolToObject(resp, "status", err == 0);

    return err;
}

int heapi_msg_DEVICES (struct HEApiClient *client, cJSON *json, cJSON *resp) {
    int err = 0;

    // RESPONSE
    cJSON *resp_devices = cJSON_AddArrayToObject(resp, "devices");

    for(int i = 0; i < HED_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] != NULL) 
            cJSON_AddItemToArray(resp_devices, hedev_to_json(device_list[i]));
    }

    return err;
}

int heapi_msg_SET_IQS_REGS (struct HEApiClient *client, cJSON *json, cJSON *resp) {

    cJSON *regs = cJSON_GetObjectItem(json, "regs");
    cJSON *device_object = cJSON_GetObjectItem(json, "device");
    cJSON *save = cJSON_GetObjectItem(json, "save");

    // RESPONSE
    if(regs == NULL || device_object == NULL || save == NULL) {
        // Fail
        printf("[heapi_msg_SET_IQS_REGS] FAIL: Null regs/device_object/save\n");
        cJSON_AddBoolToObject(resp, "status", cJSON_False);
        return 1;
    }
    // Find device
    wchar_t serial_buff[64];
    swprintf(serial_buff, 64, L"%hs", device_object->valuestring);
    struct HEDev *device = find_device(NULL, serial_buff, NULL);

    // Check device
    if(device == NULL) {
        // Fail
        printf("[heapi_msg_SET_IQS_REGS] FAIL: could not find device %ls\n", serial_buff);
        cJSON_AddBoolToObject(resp, "status", cJSON_False);
        return 1;
    }

    cJSON *current_reg = NULL;
    char *current_key = NULL;
    
    struct iqs5xx_reg_config config;
    memset(&config, 0, sizeof(struct iqs5xx_reg_config));
    
    // Set registers
    cJSON_ArrayForEach(current_reg, regs) {
        current_key = current_reg->string;
        if(current_key != NULL) {
            
            if(strcmp("activeRefreshRate", current_key) == 0) {
                config.activeRefreshRate = current_reg->valueint;
            }
            else if(strcmp("idleRefreshRate", current_key) == 0) {
                config.idleRefreshRate = current_reg->valueint;
            }
            else if(strcmp("singleFingerGestureMask", current_key) == 0) {
                config.singleFingerGestureMask = current_reg->valueint;
            }
            else if(strcmp("multiFingerGestureMask", current_key) == 0) {
                config.multiFingerGestureMask = current_reg->valueint;
            }
            else if(strcmp("tapTime", current_key) == 0) {
                config.tapTime = current_reg->valueint;
            }
            else if(strcmp("tapDistance", current_key) == 0) {
                config.tapDistance = current_reg->valueint;
            }
            else if(strcmp("touchMultiplier", current_key) == 0) {
                config.touchMultiplier = current_reg->valueint;
            }
            else if(strcmp("debounce", current_key) == 0) {
                config.debounce = current_reg->valueint;
            }
            else if(strcmp("i2cTimeout", current_key) == 0) {
                config.i2cTimeout = current_reg->valueint;
            }
            else if(strcmp("filterSettings", current_key) == 0) {
                config.filterSettings = current_reg->valueint;
            }
            else if(strcmp("filterDynBottomBeta", current_key) == 0) {
                config.filterDynBottomBeta = current_reg->valueint;
            }
            else if(strcmp("filterDynLowerSpeed", current_key) == 0) {
                config.filterDynLowerSpeed = current_reg->valueint;
            }
            else if(strcmp("filterDynUpperSpeed", current_key) == 0) {
                config.filterDynUpperSpeed = current_reg->valueint;
            }
            else if(strcmp("initScrollDistance", current_key) == 0) {
                config.initScrollDistance = current_reg->valueint;
            }
                
        }
    }

    if(zmk_control_msg_set_iqs5xx_registers(device, config, cJSON_IsTrue(save)) == 0) {
        printf("[heapi_msg_SET_IQS_REGS] Written to %s\n", device->product->product_string);
        cJSON_AddBoolToObject(resp, "status", cJSON_True);
        return 0;
    }
    else {
        printf("[heapi_msg_SET_IQS_REGS] FAIL: Write\n");
        cJSON_AddBoolToObject(resp, "status", cJSON_False);
        return 1;
    }
}

int heapi_msg_GET_IQS_REGS (struct HEApiClient *client, cJSON *json, cJSON *resp) {
    cJSON *device_object = cJSON_GetObjectItem(json, "device");

    // RESPONSE
    if(device_object == NULL) {
        // Fail
        printf("[heapi_msg_GET_IQS_REGS] FAIL: 'device' field missing from request\n");
        cJSON_AddBoolToObject(resp, "status", cJSON_False);
        return 1;
    }
    // Find device
    wchar_t serial_buff[64];
    swprintf(serial_buff, 64, L"%hs", device_object->valuestring);
    struct HEDev *device = find_device(NULL, serial_buff, NULL);

    // Check device
    if(device == NULL) {
        // Fail
        printf("[heapi_msg_GET_IQS_REGS] FAIL: could not find device %ls\n", serial_buff);
        cJSON_AddBoolToObject(resp, "status", cJSON_False);
        return 1;
    }

    uint8_t buff[128];
    memset(buff, 0, sizeof(buff));
    
    int rlen = zmk_control_get_config(device, ZMK_CONFIG_CUSTOM_IQS5XX_REGS, buff, sizeof(buff));

    if(rlen <= 0) {
        // Fail
        printf("[heapi_msg_GET_IQS_REGS] FAIL: could not read\n");
        cJSON_AddBoolToObject(resp, "status", cJSON_False);
        return 1;
    }

    struct iqs5xx_reg_config *rec_registers;

    struct zmk_control_msg_header *rec_hdr = (struct zmk_control_msg_header *)buff;
    struct zmk_control_msg_get_config *rec_config = (struct zmk_control_msg_get_config *)(buff + sizeof(struct zmk_control_msg_header));

    rec_registers = (struct iqs5xx_reg_config *)&rec_config->data;

    cJSON_AddBoolToObject(resp, "status", cJSON_True);
    cJSON *reg_obj = cJSON_AddObjectToObject(resp, "regs");

    cJSON_AddNumberToObject(reg_obj, "activeRefreshRate", rec_registers->activeRefreshRate);
    cJSON_AddNumberToObject(reg_obj, "idleRefreshRate", rec_registers->idleRefreshRate);
    cJSON_AddNumberToObject(reg_obj, "singleFingerGestureMask", rec_registers->singleFingerGestureMask);
    cJSON_AddNumberToObject(reg_obj, "multiFingerGestureMask", rec_registers->multiFingerGestureMask);
    cJSON_AddNumberToObject(reg_obj, "tapTime", rec_registers->tapTime);
    cJSON_AddNumberToObject(reg_obj, "tapDistance", rec_registers->tapDistance);
    cJSON_AddNumberToObject(reg_obj, "touchMultiplier", rec_registers->touchMultiplier);
    cJSON_AddNumberToObject(reg_obj, "debounce", rec_registers->debounce);
    cJSON_AddNumberToObject(reg_obj, "i2cTimeout", rec_registers->i2cTimeout);
    cJSON_AddNumberToObject(reg_obj, "filterSettings", rec_registers->filterSettings);
    cJSON_AddNumberToObject(reg_obj, "filterDynBottomBeta", rec_registers->filterDynBottomBeta);
    cJSON_AddNumberToObject(reg_obj, "filterDynLowerSpeed", rec_registers->filterDynLowerSpeed);
    cJSON_AddNumberToObject(reg_obj, "filterDynUpperSpeed", rec_registers->filterDynUpperSpeed);
    cJSON_AddNumberToObject(reg_obj, "initScrollDistance", rec_registers->initScrollDistance);

    return 0;
}