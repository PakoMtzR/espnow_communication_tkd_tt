#include <Arduino.h>
#include "pressure_sensor.h"

// Definición e inicialización de variables globales
int PRESSURE_THRESHOLD = 150;           // Valor por default
uint8_t pressure_sensor_pin = 34;
uint16_t adc_resolution = 4096;
float resistance_ref = 217.0f; 
std::vector<int> pressure_history;
unsigned long last_alert_time = 0;

// Implementacion de funciones
uint16_t read_pressure() {
    return analogRead(pressure_sensor_pin);
}

float calculate_difference(uint16_t current_value) {
    if (pressure_history.size() < HISTORY_SIZE) {
        return 0;
    }

    float avg_historical = 0.0f;
    for (uint16_t value : pressure_history) {
        avg_historical += value;
    }
    avg_historical /= pressure_history.size();

    float difference = abs(current_value - avg_historical);
    return difference;
}

float calculate_resistance(uint16_t current_value) {
    return current_value != 0 ? resistance_ref * (((float)adc_resolution/current_value) - 1) : 0.0f;
}

void append_to_pressure_history(uint16_t new_value) {
    pressure_history.push_back(new_value);

    // Mantener historial con tamaño fijo
    if(pressure_history.size() > HISTORY_SIZE) {
        pressure_history.erase(pressure_history.begin());
    }
}