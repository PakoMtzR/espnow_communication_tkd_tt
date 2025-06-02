#include <esp_now.h>
#include <WiFi.h>
#include "DeviceData.h"

typedef struct {
    uint8_t controllerID;
    uint8_t data;
} struct_message;

// Variables de estado
struct_message firstReceivedMessage;
bool waitingForSecondController = false;
unsigned long firstMessageTime = 0;
const unsigned long MAX_WAIT_TIME = 2000; // 500ms para recibir la coincidencia


// Callback cuando llega un mensaje
void OnDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len) {
    struct_message receivedMessage;
    memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));

    if (!waitingForSecondController) {
        // Primer mensaje recibido: empezar a esperar
        firstReceivedMessage = receivedMessage;
        firstMessageTime = millis();
        waitingForSecondController = true;
        Serial.print("Primer dato recibido (Control ");
        Serial.print(receivedMessage.controllerID);
        Serial.print("): ");
        Serial.println(receivedMessage.data);
    } 
    else {
        // Segundo mensaje recibido: verificar coincidencia
        if (receivedMessage.data == firstReceivedMessage.data && 
            receivedMessage.controllerID != firstReceivedMessage.controllerID) {
            
            unsigned long timeDiff = millis() - firstMessageTime;
            if (timeDiff <= MAX_WAIT_TIME) {
                Serial.print("¡Coincidencia! Acción: ");
                Serial.print(receivedMessage.data);
                Serial.print(", Tiempo de diferencia: ");
                Serial.print(timeDiff);
                Serial.println("ms");
            }
        }
        // Reiniciar el estado, independientemente de si hubo coincidencia
        waitingForSecondController = false;
    }
}


void setup() {
    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error al iniciar ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(OnDataReceived);
}

void loop() {
    // Si pasó el tiempo máximo y no llegó el segundo mensaje, reiniciar
    if (waitingForSecondController && (millis() - firstMessageTime > MAX_WAIT_TIME)) {
        Serial.println("Tiempo agotado. Ignorando primer dato.");
        waitingForSecondController = false;
    }
}
