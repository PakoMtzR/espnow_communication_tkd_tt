#include <esp_now.h>
#include <WiFi.h>
#include <vector>

#include "const.h"
#include "DeviceData.h"

const uint8_t pressure_sensor = 34;
const int HISTORY_SIZE = 10;
const int THRESHOLD = 150;
const int DEBOUNCE_TIME = 1000;

const float resistance_ref = 217; 

std::vector<int> pressure_history;
unsigned long last_alert_time = 0;

// Estructura de datos para enviar por ESPNOW
DeviceMessage message;
bool dataSent = false;

uint16_t read_pressure();
float calculate_difference(uint16_t current_value);
float calculate_resistance(uint16_t current_value);

// Rutina cuando se envia un mensaje por espnow
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    
    Serial.print("Entrega a ");
    Serial.print(macStr);
    Serial.print(": ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Exitosa" : "Fallida");

    if(status == ESP_NOW_SEND_SUCCESS) {
        dataSent = true;
    }
}

// Funcion para inicializar y configuracion de ESPNOW
void espnow_init() {
    WiFi.mode(WIFI_STA);
    Serial.print("MAC Transmisor: ");
    Serial.println(WiFi.macAddress());

    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        while(1) delay(100);
    }

    // Registramos el callback
    esp_now_register_send_cb(OnDataSent);

    // Emparejamos dispositivo receptor
    esp_now_peer_info_t peerInfo;
    memcpy(peerInfo.peer_addr, MAC_RECEIVER, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;
    peerInfo.ifidx = WIFI_IF_STA;
    
    if (esp_now_add_peer(&peerInfo) != ESP_OK) {
        Serial.println("Error al agregar peer");
        while(1) delay(100);
    }
}

// Funcion para el envio de datos por ESPNOW
void espnow_sendMessage() {
    esp_err_t result = esp_now_send(MAC_RECEIVER, (uint8_t *) &message, sizeof(message));
    if (result != ESP_OK) {
      Serial.println("Error al enviar");
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("Inicializado transmisor del peto...");

    message.base.deviceID = 3;
    message.base.deviceType = TRUNK_PROTECTOR_TRANSMITTER;

    esp_now_init();
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

    if ((difference > THRESHOLD) && ((millis() - last_alert_time) > DEBOUNCE_TIME) && !dataSent) {
        last_alert_time = millis();
        message.payload.trunk_protector.pressure_value = difference;
        esp_err_t result = esp_now_send(MAC_RECEIVER, (uint8_t *) &message, sizeof(message));
        Serial.printf("Lectura ADC: %d | Resistencia: %.2f | Diferencia: %.2f | Contacto detectado!\n", current_value, resistance, difference);
    } 
    else {
        Serial.printf("Lectura ADC: %d | Resistencia: %.2f | Diferencia: %.2f\n", current_value, resistance, difference);
        dataSent = false;
    } 
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