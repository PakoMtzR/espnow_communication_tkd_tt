#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

#include <stdint.h>

// Estructura de datos para enviar por espnow tanto para los controles como los transmisores del peto

// Informacion para identificar al dispositivo 
struct DeviceBase
{
    uint8_t deviceID;
    uint8_t deviceType;
};

// Datos a enviar dependiendo el tipo de dispositivo
union DevicePayload {
    struct { uint8_t data; } control_transmitter;  
    struct { float pressure_value; } trunk_protector;
};

// Unimos informacion del dispositivo asi como datos de los sensores de este
struct DeviceMessage {
    DeviceBase base;
    DevicePayload payload;
};

#endif