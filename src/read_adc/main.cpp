// Codigo para probar el ADC del ESP32

#include <Arduino.h>
#include <vector>

const uint8_t pressure_sensor = 34;
const int HISTORY_SIZE = 10;
const int THRESHOLD = 150;
const int DEBOUNCE_TIME = 1000;

const float resistance_ref = 217; 

std::vector<int> pressure_history;
unsigned long last_alert_time = 0;

uint16_t read_pressure();
float calculate_difference(uint16_t current_value);
float calculate_resistance(uint16_t current_value);

void setup() {
    Serial.begin(115200);
}

void loop() {
    // Leer valor del sensor y agregarlo al historial
    uint16_t current_value = read_pressure();
    pressure_history.push_back(current_value);

    // Mantener historial con tamaño fijo
    if(pressure_history.size() > HISTORY_SIZE) {
        pressure_history.erase(pressure_history.begin());
    }

    // Calcula la diferencia
    float difference = calculate_difference(current_value);
    float resistance = calculate_resistance(current_value);

    if ((difference > THRESHOLD) && ((millis() - last_alert_time) > DEBOUNCE_TIME)) {
        last_alert_time = millis();
        Serial.printf("Lectura ADC: %d | Resistencia: %.2f | Diferencia: %.2f | Contacto detectado!\n", current_value, resistance, difference);
    } 
    else Serial.printf("Lectura ADC: %d | Resistencia: %.2f | Diferencia: %.2f\n", current_value, resistance, difference);

    delay(100);
}

uint16_t read_pressure() {
    return analogRead(pressure_sensor);
}

float calculate_difference(uint16_t current_value) {
    if (pressure_history.size() < HISTORY_SIZE) {
        return 0;
    }

    float avg_historical = 0;
    for (uint16_t value : pressure_history) {
        avg_historical += value;
    }
    avg_historical /= pressure_history.size();

    float difference = abs(current_value - avg_historical);
    return difference;
}

float calculate_resistance(uint16_t current_value) {
    return current_value != 0 ? resistance_ref * ((4096.0/current_value) - 1) : 0;
}