#include "hedev_msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int hedev_build_header (struct hidergod_msg_header *header, enum hidergod_cmd_t cmd, uint16_t size) {
    header->reportId = 0x05;
    header->cmd = cmd;
    header->chunkOffset = 0;
    header->crc = 0;
    header->size = HIDERGOD_HEADER_SIZE + size;
    header->chunkSize = header->size;
    return 0;
}

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
    #if defined(__linux__)
    msg_data[1] = (int32_t)_time->tm_gmtoff;
    #elif defined(_WIN32)
    msg_data[1] = (int32_t)0;
    #endif

    device_write(device, buff, sizeof(buff));

    return 0;
}