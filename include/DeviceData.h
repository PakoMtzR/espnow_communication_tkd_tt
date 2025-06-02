#ifndef DEVICE_DATA_H
#define DEVICE_DATA_H

#include <stdint.h>

enum DeviceType {
    CONTROL_TRANSMITTER = 1,
    RECEIVING_DEVICE = 2,
    TRUNK_PROTECTOR_TRANSMITTER = 3
};

struct DeviceBase
{
    uint8_t deviceID;
    uint8_t deviceType;
};

union DevicePayload {
    struct { uint8_t data; } control_transmitter;  
};

struct DeviceMessage {
    DeviceBase base;
    DevicePayload payload;
};

#endif