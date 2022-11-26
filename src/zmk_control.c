
#include <time.h>
#include <string.h>
#include "hedev.h"
#include "zmk_control.h"

#if defined(_WIN32)
#define ZMK_CONTROL_USE_CUSTOM_GMT_OFFSET
#elif defined(__linux__)

#endif

uint8_t report_buffer[ZMK_CONTROL_REPORT_SIZE];

int zmk_control_build_header (struct zmk_control_msg_header *header, enum zmk_control_cmd_t cmd, uint16_t size) {
    header->report_id = 0x05;
    header->cmd = cmd;
    header->chunk_offset = 0;
    header->crc = 0;
    // Short messages are padded to ZMK_CONTROL_REPORT_DATA_SIZE
    header->size = size < ZMK_CONTROL_REPORT_DATA_SIZE ? ZMK_CONTROL_REPORT_DATA_SIZE : size;
    header->chunk_size = header->size <= ZMK_CONTROL_REPORT_DATA_SIZE ? header->size : ZMK_CONTROL_REPORT_DATA_SIZE;
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
    struct tm *ptm;

#if !defined(_WIN32)
    struct tm gbuf;
    ptm = gmtime_r(&rawtime, &gbuf);
#else
    ptm = gmtime(&rawtime);
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

    struct tm *_time = localtime(&rawtime);

    PACK(struct {
        int32_t timestamp;
        int32_t offset;
    }) _time_msg;

    _time_msg.timestamp = rawtime;
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

uint8_t msg_buffer[ZMK_CONTROL_REPORT_SIZE];

int zmk_control_write_message (struct HEDev *device, struct zmk_control_msg_header *header, uint8_t *data) {
    memset(msg_buffer, 0, sizeof(msg_buffer));
    if(header->size <= ZMK_CONTROL_REPORT_DATA_SIZE) {
        // No need for chunks - build message
        header->chunk_offset = 0;
        header->chunk_size = header->size;
        memcpy(msg_buffer, header, sizeof(struct zmk_control_msg_header));
        memcpy(msg_buffer + sizeof(struct zmk_control_msg_header), data, header->size);

        return device_write(device, msg_buffer, ZMK_CONTROL_REPORT_SIZE);
    }

    int err = 0;
    // Chunk the message
    int bytes_left = header->size;
    while(bytes_left > 0) {
        header->chunk_offset = header->size - bytes_left;
        header->chunk_size = bytes_left <= ZMK_CONTROL_REPORT_DATA_SIZE ? bytes_left : ZMK_CONTROL_REPORT_DATA_SIZE;
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

int zmk_control_read_message (struct HEDev *device, struct zmk_control_msg_header *header, uint8_t *data) {
    memset(msg_buffer, 0, sizeof(msg_buffer));
    if(header->size <= ZMK_CONTROL_REPORT_DATA_SIZE) {
        // No need for chunks - build message
        header->chunk_offset = 0;
        header->chunk_size = header->size;
        memcpy(msg_buffer, header, sizeof(struct zmk_control_msg_header));
        memcpy(msg_buffer + sizeof(struct zmk_control_msg_header), data, header->size);

        return device_write(device, msg_buffer, ZMK_CONTROL_REPORT_SIZE);
    }

    int err = 0;
    // Chunk the message
    int bytes_left = header->size;
    while(bytes_left > 0) {
        header->chunk_offset = header->size - bytes_left;
        header->chunk_size = bytes_left <= ZMK_CONTROL_REPORT_DATA_SIZE ? bytes_left : ZMK_CONTROL_REPORT_DATA_SIZE;
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