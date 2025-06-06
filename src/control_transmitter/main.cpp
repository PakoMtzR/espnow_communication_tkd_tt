#include <Arduino.h>
#include <esp_now.h>
#include <WiFi.h>
#include "const.h"
#include "DeviceData.h"

// Pines donde están conectados los botones (configuración pull-down)
const int buttonPins[] = {27, 17, 14, 13, 16, 4};
const int numButtons = sizeof(buttonPins) / sizeof(buttonPins[0]);

// Estados y variables de los botones
volatile bool buttonInterrupt[] = {false, false, false, false, false, false};
volatile bool buttonState[] = {false, false, false, false, false, false};
bool holdingBlueTrigger = false;
bool holdingRedTrigger = false;

// Anti-rebote
volatile unsigned long lastInterruptTime[] = {0, 0, 0, 0, 0, 0};
const unsigned long debounceDelay = 200; // ms

// Variable para accion del control
uint8_t data = CONTROL_STATE::NO_ACTION;

// Estructura de datos para enviar por ESPNOW
DeviceMessage message;

// Rutina cuando se envia un mensaje por espnow
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
    char macStr[18];
    snprintf(macStr, sizeof(macStr), "%02X:%02X:%02X:%02X:%02X:%02X", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
    
    Serial.print("Entrega a ");
    Serial.print(macStr);
    Serial.print(": ");
    Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Exitosa" : "Fallida");
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

// Función de interrupción común
void IRAM_ATTR handleButtonInterrupt(int buttonIndex) {
    unsigned long interruptTime = millis();
    if (interruptTime - lastInterruptTime[buttonIndex] > debounceDelay) {
        buttonState[buttonIndex] = digitalRead(buttonPins[buttonIndex]);  // Guardamos estado actual del boton
        buttonInterrupt[buttonIndex] = true;                              // Habilitamos la "bandera" de interrupcion
        lastInterruptTime[buttonIndex] = interruptTime;
    }
}

// Interrupciones para cada botón
void IRAM_ATTR buttonInterrupt0() { handleButtonInterrupt(0); }
void IRAM_ATTR buttonInterrupt1() { handleButtonInterrupt(1); }
void IRAM_ATTR buttonInterrupt2() { handleButtonInterrupt(2); }
void IRAM_ATTR buttonInterrupt3() { handleButtonInterrupt(3); }
void IRAM_ATTR buttonInterrupt4() { handleButtonInterrupt(4); }
void IRAM_ATTR buttonInterrupt5() { handleButtonInterrupt(5); }

void setup() {
    message.base.deviceType = CONTROL_TRANSMITTER;
    message.base.deviceID = 2;

    Serial.begin(115200);
    Serial.println("Inicializando Control...");

    for(uint8_t i = 0; i < numButtons; i++) {
        pinMode(buttonPins[i], INPUT);
    }

    // Configuramos interrupciones para todos los botones
    attachInterrupt(digitalPinToInterrupt(buttonPins[0]), buttonInterrupt0, CHANGE);
    attachInterrupt(digitalPinToInterrupt(buttonPins[1]), buttonInterrupt1, CHANGE);
    attachInterrupt(digitalPinToInterrupt(buttonPins[2]), buttonInterrupt2, CHANGE);
    attachInterrupt(digitalPinToInterrupt(buttonPins[3]), buttonInterrupt3, CHANGE);
    attachInterrupt(digitalPinToInterrupt(buttonPins[4]), buttonInterrupt4, CHANGE);
    attachInterrupt(digitalPinToInterrupt(buttonPins[5]), buttonInterrupt5, CHANGE);

    espnow_init();
}

void loop() {
    // Procesamos los botones de acción primero (2-5)
    for (uint8_t i = 2; i < numButtons; i++) {
        if (buttonInterrupt[i]) {
            buttonInterrupt[i] = false; // Deshabilitamos la "bandera" de interrupcion
            
            // Solo en flanco de subida (PRESS)
            if (buttonState[i]) { 
                switch (i) {
                case 2:
                    data = holdingBlueTrigger ? CONTROL_STATE::BLUE_BODY_TECHNICAL_KICK : CONTROL_STATE::BLUE_BODY_KICK;
                    holdingBlueTrigger = false;
                    break;
                case 3:
                    data = holdingBlueTrigger ? CONTROL_STATE::BLUE_HEAD_TECHNICAL_KICK : CONTROL_STATE::BLUE_HEAD_KICK;
                    holdingBlueTrigger = false;
                    break;
                case 4:
                    data = holdingRedTrigger ? CONTROL_STATE::RED_BODY_TECHNICAL_KICK : CONTROL_STATE::RED_BODY_KICK;
                    holdingRedTrigger = false;
                    break;
                case 5:
                    data = holdingRedTrigger ? CONTROL_STATE::RED_HEAD_TECHNICAL_KICK : CONTROL_STATE::RED_HEAD_KICK;
                    holdingRedTrigger = false;
                    break;
                default:
                    break;
                }
            }
        }
    }

    // Procesamos los triggers (Pines 0 y 1)
    for (uint8_t i = 0; i < 2; i++) {
        if (buttonInterrupt[i]) {
            buttonInterrupt[i] = false;
            
            if (buttonState[i]) { // Buton presionado
                if (i == 0) holdingBlueTrigger = true;
                else holdingRedTrigger = true;
            } 
            else { // Buton liberado
                if (i == 0 && holdingBlueTrigger) {
                    data = CONTROL_STATE::BLUE_PUNCH;
                    holdingBlueTrigger = false;
                }
                if (i == 1 && holdingRedTrigger) {
                    data = CONTROL_STATE::RED_PUNCH;
                    holdingRedTrigger = false;
                }
            }
        }
    }

    // Envio de datos
    if (data != CONTROL_STATE::NO_ACTION) {
        message.payload.control_transmitter.data = data;
        espnow_sendMessage();
        Serial.printf("Data: %d\n", data);
        data = CONTROL_STATE::NO_ACTION;
    }
    delay(10);
}