
#include <time.h>
#include <string.h>
#include "hedev.h"
#include "zmk_control.h"

#if defined(_WIN32)
#define ZMK_CONTROL_USE_CUSTOM_GMT_OFFSET
#elif defined(__linux__)

#endif

uint8_t report_buffer[ZMK_CONTROL_REPORT_SIZE];

size_t hex_to_bytes (const char *hex_string, uint8_t **bytes) {
    size_t hex_len = strlen(hex_string);
    if(hex_len % 2 != 0) {
        printf("[WARN]: Invalid hex string format\n");
        return 0;
    }

    *bytes = malloc(hex_len / 2);
    if(*bytes == NULL) {
        printf("[ERROR]: Out of memory\n");
        return 0;   
    }

    for(int i = 0; i < (int)hex_len / 2; i++) {
        char hx[3] = { hex_string[i * 2], hex_string[i * 2 + 1], 0 };
        (*bytes)[i] = (uint8_t)strtoul(hx, NULL, 16);
    }

    return hex_len / 2;
}

char *bytes_to_hex (uint8_t *bytes, size_t len) {
    char *hex = malloc(len * 2 + 1);
    if(hex == NULL) {
        printf("[ERROR]: Out of memory\n");
        return NULL;
    }
    hex[len * 2] = 0;

    for(int i = 0; i < (int)len; i++) {
        snprintf(&hex[i * 2], 3, "%02X", bytes[i]);
    }

    return hex;
}


int zmk_control_build_header (struct zmk_control_msg_header *header, enum zmk_control_cmd_t cmd, uint16_t size) {
    header->report_id = 0x05;
    header->cmd = cmd;
    header->chunk_offset = 0;
    header->crc = 0;
    // Short messages are padded to ZMK_CONTROL_REPORT_DATA_SIZE
    header->size = size < ZMK_CONTROL_REPORT_DATA_SIZE ? ZMK_CONTROL_REPORT_DATA_SIZE : size;
    header->chunk_size = header->size <= ZMK_CONTROL_REPORT_DATA_SIZE ? (uint8_t)header->size : (uint8_t)ZMK_CONTROL_REPORT_DATA_SIZE;
    return 0;
}

#ifdef ZMK_CONTROL_USE_CUSTOM_GMT_OFFSET
/**
 * @brief Get timezone offset. tm_gmtoff not available for windows.
 * @ref https://stackoverflow.com/a/44063597
 * @return int 
 */
int _zmk_control_get_gmt_offset () {
    time_t gmt, rawtime = time(NULL);
    struct tm *ptm = NULL;

    struct tm gbuf;
#if !defined(_WIN32)
    ptm = gmtime_r(&rawtime, &gbuf);
#else
    errno_t err = gmtime_s(&gbuf, &rawtime);
    if(!err)
        ptm = &gbuf;
#endif
    // Request that mktime() looksup dst in timezone database
    ptm->tm_isdst = -1;
    gmt = mktime(ptm);

    return (int)difftime(rawtime, gmt);
}
#endif

int zmk_control_msg_set_time (struct HEDev *device) {
    time_t rawtime;

    time(&rawtime);

    struct tm *_time = NULL;
    struct tm _tm;
    #if !defined(_WIN32)
        _time = localtime_r(&rawtime, &_tm);
    #else
        errno_t err = localtime_s(&_tm, &rawtime);
        if(!err)
            _time = &_tm;
    #endif

    PACK(struct {
        int32_t timestamp;
        int32_t offset;
    }) _time_msg;

    _time_msg.timestamp = (int32_t)rawtime;
    #ifndef ZMK_CONTROL_USE_CUSTOM_GMT_OFFSET
    _time_msg.offset = (int32_t)_time->tm_gmtoff;
    #else
    _time_msg.offset = (int32_t)_zmk_control_get_gmt_offset();
    #endif

    // Do not save datetime
    return zmk_control_set_config(device, ZMK_CONFIG_KEY_DATETIME, &_time_msg, sizeof(_time_msg), 0);
}

int zmk_control_msg_set_mouse_sensitivity (struct HEDev *device, uint8_t sensitivity) {

    uint8_t sens = sensitivity;

    return zmk_control_set_config(device, ZMK_CONFIG_KEY_MOUSE_SENSITIVITY, &sens, sizeof(sens), 1);
}

int zmk_control_msg_set_iqs5xx_registers (struct HEDev *device, struct iqs5xx_reg_config config, uint8_t save) {
    struct iqs5xx_reg_config conf = config;

    return zmk_control_set_config(device, ZMK_CONFIG_CUSTOM_IQS5XX_REGS, &conf, sizeof(conf), save);
}

int zmk_control_set_config (struct HEDev *device, uint16_t key, void *data, uint16_t len, uint8_t save) {
    // Output buffer
    uint8_t buff[4092];

    // Header
    struct zmk_control_msg_header header;
    zmk_control_build_header(&header, ZMK_CONTROL_CMD_SET_CONFIG, sizeof(struct zmk_control_msg_set_config) - 1 + len);

    struct zmk_control_msg_set_config *msg = (struct zmk_control_msg_set_config*)(buff);
    uint8_t *msg_data = (uint8_t*)&msg->data;
    // Set save flag
    msg->save = save;
    msg->key = key;
    msg->size = len;
    memcpy(msg_data, data, len);

    zmk_control_write_message(device, &header, buff);

    return 0;
}

int zmk_control_get_config (struct HEDev *device, uint16_t key, void *data, uint16_t maxlen) {
    // Output buffer
    uint8_t buff[32];
    // Header
    struct zmk_control_msg_header header;
    zmk_control_build_header(&header, ZMK_CONTROL_CMD_GET_CONFIG, 1);

    struct zmk_control_msg_get_config *msg = (struct zmk_control_msg_get_config*)(buff);
    uint8_t *msg_data = (uint8_t*)&msg->data;
    msg->key = key;
    msg->size = maxlen;
    *msg_data = 0;

    // Write a message
    zmk_control_write_message(device, &header, buff);
    
    // Read response
    int len = device_read(device, data, maxlen);

    struct zmk_control_msg_get_config *resp = (struct zmk_control_msg_get_config*)data;
    if(resp->key == key) {
        // Offset data by -4, since the first 4 bytes are from struct zmk_control_msg_get_config
        memmove(data, (uint8_t*)data + 4, len - 4);
        return len - 4;
    }

    return 0;
}

uint8_t msg_buffer[ZMK_CONTROL_REPORT_SIZE];

int zmk_control_write_message (struct HEDev *device, struct zmk_control_msg_header *header, uint8_t *data) {
    memset(msg_buffer, 0, sizeof(msg_buffer));
    if(header->size <= ZMK_CONTROL_REPORT_DATA_SIZE) {
        // No need for chunks - build message
        header->chunk_offset = 0;
        header->chunk_size = (uint8_t)header->size;
        memcpy(msg_buffer, header, sizeof(struct zmk_control_msg_header));
        memcpy(msg_buffer + sizeof(struct zmk_control_msg_header), data, header->size);

        return device_write(device, msg_buffer, ZMK_CONTROL_REPORT_SIZE);
    }

    int err = 0;
    // Chunk the message
    int bytes_left = header->size;
    while(bytes_left > 0) {
        header->chunk_offset = header->size - (uint16_t)bytes_left;
        header->chunk_size = bytes_left <= (int)ZMK_CONTROL_REPORT_DATA_SIZE ? (uint8_t)bytes_left : (uint8_t)ZMK_CONTROL_REPORT_DATA_SIZE;
        memcpy(msg_buffer, header, sizeof(struct zmk_control_msg_header));
        memcpy(msg_buffer + sizeof(struct zmk_control_msg_header), data + header->chunk_offset, header->chunk_size);
        err = device_write(device, msg_buffer, ZMK_CONTROL_REPORT_SIZE);
        if(err < 0) {
            break;
        }

        bytes_left -= header->chunk_size;
    }

    return err;
}
