#ifndef PRESSURE_SENSOR_H
#define PRESSURE_SENSOR_H

#include <stdint.h>
#include <vector>

// Constantes
constexpr int HISTORY_SIZE = 10;
constexpr int PRESSURE_THRESHOLD = 150;
constexpr int DEBOUNCE_TIME = 1000;

// Declaración de variables (definidas en el .cpp)
extern uint8_t pressure_sensor_pin;
extern uint16_t adc_resolution;
extern float resistance_ref; 
extern std::vector<int> pressure_history;
extern unsigned long last_alert_time;

// Declaracion de funciones
uint16_t read_pressure();
float calculate_difference(uint16_t current_value);
float calculate_resistance(uint16_t current_value);
void append_to_pressure_history(uint16_t new_value);

#endif