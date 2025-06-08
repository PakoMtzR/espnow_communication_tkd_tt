#ifndef DIP_SWITCH_H
#define DIP_SWITCH_H

#include <stdint.h>

// Definir los pines donde está conectado el DIP switch
#define DIP_SWITCH_PIN_1 12
#define DIP_SWITCH_PIN_2 14
#define DIP_SWITCH_PIN_3 27
#define DIP_SWITCH_PIN_4 26

void dipswitch_config();
uint8_t read_dipswitch();

#endif