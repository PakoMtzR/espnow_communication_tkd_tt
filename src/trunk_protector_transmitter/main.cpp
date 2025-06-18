#include <esp_now.h>
#include <WiFi.h>
#include <vector>

#include "const.h"
#include "DataStructure.h"
#include "pressure_sensor.h"
#include "dip_switch.h"

// Variable que almacenará el valor del DIP switch
uint8_t dipValue = 0;

// Estructura de datos para enviar por ESPNOW
DeviceMessage message;
bool dataSent = false;

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
    message.base.deviceID = 3;
    message.base.deviceType = TRUNK_PROTECTOR_TRANSMITTER;
    message.payload.trunk_protector.player_color = PLAYER_BLUE;

    Serial.begin(115200);
    Serial.println("Inicializado transmisor del peto...");

    dipswitch_config();
    dipValue = read_dipswitch();

    Serial.printf("Valor leído del DIP switch: 0x%02X\n", dipValue);

    if (dipValue != 0) {
        // Configurar el modo de operacion basado en DIP switch
        PRESSURE_THRESHOLD = dipValue * 50;
        Serial.printf("Valor threshold: %u\n", PRESSURE_THRESHOLD);
    }

    espnow_init();
}

void loop() {
    // Leer valor del sensor y agregarlo al historial
    uint16_t current_value = read_pressure();
    append_to_pressure_history(current_value);

    // Calcula la diferencia y la resistencia del sensor
    float difference = calculate_difference(current_value);
    float resistance = calculate_resistance(current_value);

    if ((difference > PRESSURE_THRESHOLD) && ((millis() - last_alert_time) > DEBOUNCE_TIME) && !dataSent) {
        last_alert_time = millis();
        Serial.printf("Lectura ADC: %d | Resistencia: %.2f | Diferencia: %.2f | Contacto detectado!\n", current_value, resistance, difference);
        
        message.payload.trunk_protector.pressure_value = difference;
        esp_err_t result = esp_now_send(MAC_RECEIVER, (uint8_t *) &message, sizeof(message));
    } 
    else {
        Serial.printf("Lectura ADC: %d | Resistencia: %.2f | Diferencia: %.2f\n", current_value, resistance, difference);
        dataSent = false;
    } 
    delay(100);
}