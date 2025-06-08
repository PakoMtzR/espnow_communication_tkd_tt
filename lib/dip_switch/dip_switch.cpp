#include <Arduino.h>
#include "dip_switch.h"

// Configurar los pines del DIP switch como entradas con pull-up
void dipswitch_config() {
    pinMode(DIP_SWITCH_PIN_1, INPUT_PULLUP);
    pinMode(DIP_SWITCH_PIN_2, INPUT_PULLUP);
    pinMode(DIP_SWITCH_PIN_3, INPUT_PULLUP);
    pinMode(DIP_SWITCH_PIN_4, INPUT_PULLUP);
}

// Función para leer el estado del DIP switch
uint8_t read_dipswitch() {
    uint8_t dipValue = 0;
    if (digitalRead(DIP_SWITCH_PIN_1) == LOW) dipValue |= (1 << 0);
    if (digitalRead(DIP_SWITCH_PIN_2) == LOW) dipValue |= (1 << 1);
    if (digitalRead(DIP_SWITCH_PIN_3) == LOW) dipValue |= (1 << 2);
    if (digitalRead(DIP_SWITCH_PIN_4) == LOW) dipValue |= (1 << 3);

    return dipValue;
}