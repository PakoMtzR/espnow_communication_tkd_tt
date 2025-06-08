// Codigo para probar el ADC del ESP32
#include <Arduino.h>
#include "pressure_sensor.h"

void setup() {
    Serial.begin(115200);
    Serial.printf("\nInicializando lector ADC...");
}

void loop() {
    // Leer valor del sensor y agregarlo al historial
    uint16_t current_value = read_pressure();
    
    append_to_pressure_history(current_value);

    // Calcula la diferencia
    float difference = calculate_difference(current_value);
    float resistance = calculate_resistance(current_value);

    if ((difference > PRESSURE_THRESHOLD) && ((millis() - last_alert_time) > DEBOUNCE_TIME)) {
        last_alert_time = millis();
        Serial.printf("Lectura ADC: %d | Resistencia: %.2f | Diferencia: %.2f | Contacto detectado!\n", current_value, resistance, difference);
    } 
    else Serial.printf("Lectura ADC: %d | Resistencia: %.2f | Diferencia: %.2f\n", current_value, resistance, difference);

    delay(100);
}