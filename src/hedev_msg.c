#include "hedev_msg.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int hedev_build_header (struct hidergod_msg_header *header, enum hidergod_cmd_t cmd, uint16_t size) {
    header->reportId = 0;
    header->cmd = cmd;
    header->chunkOffset = 0;
    header->crc = 0;
    header->size = sizeof(struct hidergod_msg_header);
    header->chunkSize = header->size;
    return 0;
}

int hedev_set_time (struct HEDev *device) {
    time_t rawtime;

    time(&rawtime);

    uint8_t buff[sizeof(struct hidergod_msg_header) + sizeof(struct hidergod_msg_set_value) + (sizeof(int) - 1)];

    struct hidergod_msg_header *header = (struct hidergod_msg_header*)(buff + 0);
    struct hidergod_msg_set_value *msg = (struct hidergod_msg_set_value*)(buff + sizeof(struct hidergod_msg_header));
    int *value = (int*)&msg->data;

    hedev_build_header(header, HIDERGOD_CMD_SET_VALUE, sizeof(struct hidergod_msg_set_value) + (sizeof(int) - 1));
    
    msg->key = HIDERGOD_VALUE_KEY_TIME;
    msg->length = 4;
    *value = (int)rawtime;

    device_write(device, buff, sizeof(buff));

    return 0;
}