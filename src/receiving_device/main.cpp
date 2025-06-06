#include <esp_now.h>
#include <WiFi.h>
#include "DeviceData.h"
#include "const.h"

// Definir los pines donde está conectado el DIP switch
#define DIP_SWITCH_PIN_1 12
#define DIP_SWITCH_PIN_2 14
#define DIP_SWITCH_PIN_3 27
#define DIP_SWITCH_PIN_4 26
// Añade más pines si tu DIP switch tiene más posiciones

// Variable que almacenará el valor del DIP switch
uint8_t dipValue = 0;

// Variables de estado
DeviceMessage firstReceivedMessage;
bool waitingForSecondController = false;
unsigned long firstMessageTime = 0;
const unsigned long MAX_WAIT_TIME = 2000; // 2000ms para recibir la coincidencia

const bool DEBUG_MODE = true;
uint8_t RECEIVING_DEVICE_MODE = ONE_CONTROL;

// Callback cuando llega un mensaje
void OnDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len) {
    DeviceMessage receivedMessage;
    memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));

    switch (RECEIVING_DEVICE_MODE)
    {
        case ONE_CONTROL:
            if (receivedMessage.base.deviceType == CONTROL_TRANSMITTER) {
                if (DEBUG_MODE) {
                    Serial.print("¡Recibido! Dato: ");
                    Serial.println(receivedMessage.payload.control_transmitter.data);
                }
                else Serial.println(receivedMessage.payload.control_transmitter.data);
            }
            break;

        case TWO_CONTROL:
            if (receivedMessage.base.deviceType == CONTROL_TRANSMITTER) {
                if (!waitingForSecondController) {
                // Primer mensaje recibido: empezar a esperar
                firstReceivedMessage = receivedMessage;
                firstMessageTime = millis();
                waitingForSecondController = true;
                Serial.print("Primer dato recibido (Control ");
                Serial.print(receivedMessage.base.deviceID);
                Serial.print("): ");
                Serial.println(receivedMessage.payload.control_transmitter.data);
                } 
                else {
                    // Segundo mensaje recibido: verificar coincidencia
                    if (receivedMessage.payload.control_transmitter.data == firstReceivedMessage.payload.control_transmitter.data && 
                        receivedMessage.base.deviceID != firstReceivedMessage.base.deviceID) {
                        
                        unsigned long timeDiff = millis() - firstMessageTime;
                        if (timeDiff <= MAX_WAIT_TIME) {
                            if (DEBUG_MODE) {
                                Serial.print("¡Coincidencia! Acción: ");
                                Serial.print(receivedMessage.payload.control_transmitter.data);
                                Serial.print(", Tiempo de diferencia: ");
                                Serial.print(timeDiff);
                                Serial.println("ms");
                            }
                            else Serial.println(receivedMessage.payload.control_transmitter.data);
                        }
                    }
                    // Reiniciar el estado, independientemente de si hubo coincidencia
                    waitingForSecondController = false;
                }
            }
            break;
        
        case JUST_TRUNK_PROTECTOR:
            if (receivedMessage.base.deviceType == TRUNK_PROTECTOR_TRANSMITTER) {
                if (DEBUG_MODE) {
                    Serial.print("Contacto!, Fuerza: ");
                    Serial.println(receivedMessage.payload.trunk_protector.pressure_value);
                }
                else Serial.println(receivedMessage.payload.trunk_protector.pressure_value);
            }
            break;
        
        default:
            break;
    }
}


void setup() {
    Serial.begin(115200);
    Serial.println("Inicializando Receptor...");
    Serial.print("MAC Receptor: ");
    Serial.println(WiFi.macAddress());

    // Configurar los pines del DIP switch como entradas con pull-up
    pinMode(DIP_SWITCH_PIN_1, INPUT_PULLUP);
    pinMode(DIP_SWITCH_PIN_2, INPUT_PULLUP);
    pinMode(DIP_SWITCH_PIN_3, INPUT_PULLUP);
    pinMode(DIP_SWITCH_PIN_4, INPUT_PULLUP);
    
    // Leer cada switch y asignar el valor correspondiente al bit
    if(digitalRead(DIP_SWITCH_PIN_1) == LOW) dipValue |= 0b0000;
    if(digitalRead(DIP_SWITCH_PIN_2) == LOW) dipValue |= 0b0001;
    if(digitalRead(DIP_SWITCH_PIN_3) == LOW) dipValue |= 0b0011;
    if(digitalRead(DIP_SWITCH_PIN_4) == LOW) dipValue |= 0b0100;

    Serial.print("Valor leído del DIP switch: ");
    Serial.println(dipValue);

    switch (dipValue)
    {
        case 0b0001:
            RECEIVING_DEVICE_MODE = ONE_CONTROL;
            break;
        case 0b0010:
            RECEIVING_DEVICE_MODE = TWO_CONTROL;
            break;
        case 0b0100:
            RECEIVING_DEVICE_MODE = JUST_TRUNK_PROTECTOR;
            break;
        default:
            break;
    }

    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error al iniciar ESP-NOW");
        return;
    }
    esp_now_register_recv_cb(OnDataReceived);
}

void loop() {
    switch (RECEIVING_DEVICE_MODE) {
        case ONE_CONTROL:
            break;

        case TWO_CONTROL:
            // Si pasó el tiempo máximo y no llegó el segundo mensaje, reiniciar
            if (waitingForSecondController && (millis() - firstMessageTime > MAX_WAIT_TIME)) {
                if (DEBUG_MODE) Serial.println("Tiempo agotado. Ignorando primer dato.");

                waitingForSecondController = false;
            }
            break;
        
        default:
            break;
    }
}
