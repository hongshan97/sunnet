#pragma once

#include <stdint.h>

class Conn {
public:
    enum TYPE {
        LISTEN = 1,
        CLIENT = 2,
    };

    uint8_t type;
    int fd;
    uint32_t serviceId;
};