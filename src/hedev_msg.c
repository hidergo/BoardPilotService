#include "hedev_msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#if defined(_WIN32)
#define HEDEV_USE_CUSTOM_GMT_OFFSET
#elif defined(__linux__)

#endif

uint8_t report_buffer[HIDERGOD_REPORT_SIZE];

int hedev_build_header (struct hidergod_msg_header *header, enum hidergod_cmd_t cmd, uint16_t size) {
    header->reportId = 0x05;
    header->cmd = cmd;
    header->chunkOffset = 0;
    header->crc = 0;
    header->size = HIDERGOD_HEADER_SIZE + size;
    header->chunkSize = HIDERGOD_REPORT_SIZE;
    return 0;
}

#ifdef HEDEV_USE_CUSTOM_GMT_OFFSET
/**
 * @brief Get timezone offset. tm_gmtoff not available for windows.
 * @ref https://stackoverflow.com/a/44063597
 * @return int 
 */
int _hedev_get_gmt_offset () {
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

int hedev_set_time (struct HEDev *device) {
    time_t rawtime;

    time(&rawtime);

    struct tm *_time = localtime(&rawtime);

    uint8_t buff[sizeof(struct hidergod_msg_header) + sizeof(struct hidergod_msg_set_value) - 1 + (sizeof(int) * 2)];

    struct hidergod_msg_header *header = (struct hidergod_msg_header*)(buff + 0);
    struct hidergod_msg_set_value *msg = (struct hidergod_msg_set_value*)(buff + sizeof(struct hidergod_msg_header));

    int32_t *msg_data = (int32_t*)&msg->data;

    hedev_build_header(header, HIDERGOD_CMD_SET_VALUE, sizeof(struct hidergod_msg_set_value) - 1 + (sizeof(int) * 2));
    
    msg->key = HIDERGOD_VALUE_KEY_TIME;
    msg->length = sizeof(int32_t) * 2;
    msg_data[0] = (int32_t)rawtime;
    #ifndef HEDEV_USE_CUSTOM_GMT_OFFSET
    msg_data[1] = (int32_t)_time->tm_gmtoff;
    #else
    msg_data[1] = (int32_t)_hedev_get_gmt_offset();
    #endif

    device_write(device, buff, sizeof(buff));

    return 0;
}