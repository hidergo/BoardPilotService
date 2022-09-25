#include "hedev_msg.h"

int hedev_build_header (struct hidergod_msg_header *header, enum hidergod_cmd_t cmd) {

    header->cmd = cmd;
    header->chunkOffset = 0;
    header->crc = 0;
    header->size = sizeof(struct hidergod_msg_header);
    header->chunkSize = header->size;
    return 0;
}