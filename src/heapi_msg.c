#include "heapi_msg.h"
#include "hedev.h"
#include "hedef.h"
#include "zmk_control.h"
#include <stdlib.h>
#include <string.h>

#if defined(_WIN32)
#define UNUSED UNREFERENCED_PARAMETER
#else
#define UNUSED(x) (void)(x)
#endif

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
    UNUSED(client);
    UNUSED(json);
    

    int err = 0;

    // RESPONSE
    cJSON *resp_devices = cJSON_AddArrayToObject(resp, "devices");

    for(int i = 0; i < HED_DEVICE_ALLOC_SIZE; i++) {
        if(device_list[i] != NULL) 
            cJSON_AddItemToArray(resp_devices, hedev_to_json(device_list[i]));
    }

    return err;
}

int heapi_msg_ZMK_CONTROL_WRITE (struct HEApiClient *client, cJSON *json, cJSON *resp) {
    UNUSED(client);
    
    cJSON *device_object = cJSON_GetObjectItem(json, "device");
    cJSON *save = cJSON_GetObjectItem(json, "save");
    cJSON *field = cJSON_GetObjectItem(json, "field");
    cJSON *hex = cJSON_GetObjectItem(json, "bytes");

    // RESPONSE
    if(device_object == NULL || save == NULL || hex == NULL || field == NULL) {
        // Fail
        printf("[heapi_msg_ZMK_CONTROL_WRITE] FAIL: 'device'/'save'/'bytes'/'field' field missing from request\n");
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
        printf("[heapi_msg_SET_MOUSE_SENSITIVITY] FAIL: could not find device %ls\n", serial_buff);
        cJSON_AddBoolToObject(resp, "status", cJSON_False);
        return 1;
    }

    uint8_t *bytes = NULL;
    size_t bytes_len = hex_to_bytes(hex->valuestring, &bytes);
    if(bytes_len <= 0) {
        // Fail
        printf("[heapi_msg_ZMK_CONTROL_WRITE] FAIL: parse bytes\n");
        cJSON_AddBoolToObject(resp, "status", cJSON_False);
        return 1;
    }

    int err = zmk_control_set_config(device, (uint16_t)field->valueint, bytes, (uint16_t)bytes_len, (uint8_t)cJSON_IsTrue(save));

    free(bytes);
    if(err < 0) {
        // Fail
        printf("[heapi_msg_ZMK_CONTROL_WRITE] FAIL: could not write\n");
        cJSON_AddBoolToObject(resp, "status", cJSON_False);
        return 1;
    }

    cJSON_AddBoolToObject(resp, "status", cJSON_True);
    cJSON_AddStringToObject(resp, "device", device_object->valuestring);
    cJSON_AddStringToObject(resp, "field", field->valuestring);
    return 0;
}

int heapi_msg_ZMK_CONTROL_READ (struct HEApiClient *client, cJSON *json, cJSON *resp) {
    UNUSED(client);
    
    cJSON *device_object = cJSON_GetObjectItem(json, "device");
    cJSON *field = cJSON_GetObjectItem(json, "field");

    // RESPONSE
    if(device_object == NULL || field == NULL) {
        // Fail
        printf("[heapi_msg_ZMK_CONTROL_READ] FAIL: 'device'/'field' field missing from request\n");
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
        printf("[heapi_msg_ZMK_CONTROL_READ] FAIL: could not find device %ls\n", serial_buff);
        cJSON_AddBoolToObject(resp, "status", cJSON_False);
        return 1;
    }

    uint8_t buffer[ZMK_CONTROL_MAX_MESSAGE_LENGTH];
    
    int rlen = zmk_control_get_config(device, (uint16_t)field->valueint, buffer, sizeof(buffer));

    if(rlen <= 0) {
        // Fail
        printf("[heapi_msg_ZMK_CONTROL_READ] FAIL: could not read\n");
        cJSON_AddBoolToObject(resp, "status", cJSON_False);
        return 1;
    }

    char *buffer_hex = bytes_to_hex(buffer, rlen);

    cJSON_AddBoolToObject(resp, "status", cJSON_True);
    cJSON_AddStringToObject(resp, "device", device_object->valuestring);
    cJSON_AddStringToObject(resp, "field", field->valuestring);
    cJSON_AddStringToObject(resp, "data", buffer_hex);

    free(buffer_hex);

    return 0;
}
