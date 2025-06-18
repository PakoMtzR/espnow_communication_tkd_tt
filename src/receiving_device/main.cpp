#include <esp_now.h>
#include <WiFi.h>

#include "DataStructure.h"
#include "const.h"
#include "dip_switch.h"

// Variables y constantes de los leds
const uint8_t pinLedBlue = 23;
const uint8_t pinLedRed = 22;
const unsigned long MAX_WAIT_TIME_LED = 1000;
unsigned long ledBlueTurnOnTime = 0;
unsigned long ledRedTurnOnTime = 0;

// Variable que almacenará el valor del DIP switch
uint8_t dipValue = 0;

// Variables de estado mensajes de espnow
DeviceMessage firstReceivedMessage;
unsigned long firstMessageTime = 0;
const unsigned long MAX_WAIT_TIME = 2000; // 2000ms para recibir la coincidencia

// Variables para RECEIVING_DEVICE_MODE: TWO_CONTROL
bool waitingForSecondControllerConfirmation = false;

// Variables para RECEIVING_DEVICE_MODE: TRUNK_PROTECTOR_AND_CONTROL
uint8_t directActions[] = {BLUE_HEAD_KICK, BLUE_HEAD_TECHNICAL_KICK, RED_HEAD_KICK, RED_HEAD_TECHNICAL_KICK};
bool waitingForControllerConfirmation_blue = false;
bool waitingForControllerConfirmation_red = false;

// Variables de operacion del receptor
bool DEBUG_MODE = true;
uint8_t RECEIVING_DEVICE_MODE = ONE_CONTROL;

// Prototipos de funciones
void espnow_init();
void turnOnPlayerLed(uint8_t data);
void turnOffPlayerLed();

// Callback cuando llega un mensaje
void OnDataReceived(const uint8_t *mac, const uint8_t *incomingData, int len) {
    DeviceMessage receivedMessage;
    memcpy(&receivedMessage, incomingData, sizeof(receivedMessage));

    switch (RECEIVING_DEVICE_MODE) {
        case ONE_CONTROL:
            if (receivedMessage.base.deviceType == CONTROL_TRANSMITTER) {
                if (DEBUG_MODE) {
                    Serial.printf("¡Recibido!, (Control %u): %u\n", receivedMessage.base.deviceID, receivedMessage.payload.control_transmitter.data);
                }
                else Serial.printf("%u\n", receivedMessage.payload.control_transmitter.data);

                turnOnPlayerLed(receivedMessage.payload.control_transmitter.data);
            }
            break;

        case TWO_CONTROL:
            if (receivedMessage.base.deviceType == CONTROL_TRANSMITTER) {
                if (!waitingForSecondControllerConfirmation) {
                    // Primer mensaje recibido: empezar a esperar
                    firstReceivedMessage = receivedMessage;
                    firstMessageTime = millis();
                    waitingForSecondControllerConfirmation = true;

                    if (DEBUG_MODE) Serial.printf("¡Recibido! (Control %u): %u, Esperando confirmacion...\n", receivedMessage.base.deviceID, receivedMessage.payload.control_transmitter.data);
                } 
                else {
                    // Segundo mensaje recibido: verificar coincidencia
                    if (receivedMessage.payload.control_transmitter.data == firstReceivedMessage.payload.control_transmitter.data && 
                        receivedMessage.base.deviceID != firstReceivedMessage.base.deviceID) {
                        
                        // Verificamos diferencia de tiempo
                        unsigned long timeDiff = millis() - firstMessageTime;   
                        if (timeDiff <= MAX_WAIT_TIME) {
                            if (DEBUG_MODE) {
                                Serial.printf("¡Coincidencia! Acción: %u, Tiempo de diferencia: %ums\n", receivedMessage.payload.control_transmitter.data, timeDiff);
                            }
                            else Serial.printf("%u\n", receivedMessage.payload.control_transmitter.data);
                            
                            turnOnPlayerLed(receivedMessage.payload.control_transmitter.data);
                        }
                    }
                    // else Serial.printf("Primer dato ignorado por inconsistencia.\n");

                    // Reiniciar el estado, independientemente de si hubo coincidencia
                    waitingForSecondControllerConfirmation = false;
                }
            }
            break;
        
        case JUST_TRUNK_PROTECTOR:
            if (receivedMessage.base.deviceType == TRUNK_PROTECTOR_TRANSMITTER) {
                if (!DEBUG_MODE) {
                    switch (receivedMessage.payload.trunk_protector.player_color) {
                        case 0:
                            Serial.printf("%u\n", RED_PUNCH);
                            turnOnPlayerLed(RED_PUNCH);
                            break;
                        case 1:
                            Serial.printf("%u\n", BLUE_PUNCH);
                            turnOnPlayerLed(BLUE_PUNCH);
                            break;
                        default:
                            break;                    
                    }   
                }
                else Serial.printf("Contacto! (Peto %u Color: %u), Fuerza: %.2f\n", receivedMessage.base.deviceID, receivedMessage.payload.trunk_protector.player_color, receivedMessage.payload.trunk_protector.pressure_value);
                
                uint8_t data = receivedMessage.payload.trunk_protector.player_color ? RED_PUNCH : BLUE_PUNCH;
                turnOnPlayerLed(data);
            }
            break;
        
        case TRUNK_PROTECTOR_OR_CONTROL:
            if (receivedMessage.base.deviceType == TRUNK_PROTECTOR_TRANSMITTER) {
                if (!DEBUG_MODE) {
                    switch (receivedMessage.payload.trunk_protector.player_color) {
                        case 0:
                            Serial.printf("%u\n", RED_PUNCH);
                            break;
                        case 1:
                            Serial.printf("%u\n", BLUE_PUNCH);
                            break;
                        default:
                            break;                    
                    }   
                }
                else Serial.printf("Contacto (Peto %u Color: %u)!, Fuerza: %.2f\n", receivedMessage.base.deviceID, receivedMessage.payload.trunk_protector.player_color, receivedMessage.payload.trunk_protector.pressure_value);
            }
            if (receivedMessage.base.deviceType == CONTROL_TRANSMITTER) {
                uint8_t data = receivedMessage.payload.control_transmitter.data;
                data = (data == BLUE_BODY_KICK || data == BLUE_BODY_TECHNICAL_KICK || data == RED_BODY_KICK || data == RED_BODY_TECHNICAL_KICK) 
                       ? data - 1 : data;
                if (DEBUG_MODE) {
                    Serial.printf("(Control %u) - Complemento de puntos, Accion: %u\n", receivedMessage.base.deviceID, data);
                }
                else Serial.printf("%u\n", data);

                turnOnPlayerLed(data);
            }
            break;

        case TRUNK_PROTECTOR_WITH_CONTROL_CONFIRMATION:
            if (receivedMessage.base.deviceType == TRUNK_PROTECTOR_TRANSMITTER) {
                switch (receivedMessage.payload.trunk_protector.player_color) {
                    case 0:
                        // En caso de recibir dato por parte del PETO AZUL
                        waitingForControllerConfirmation_red = true;
                        break;
                    case 1:
                        // En caso de recibir dato por parte del PETO ROJO
                        waitingForControllerConfirmation_blue = true;
                        break;
                    default:
                        break;
                }

                firstMessageTime = millis();
                if (DEBUG_MODE) Serial.printf("Contacto Detectado!, (Peto %u Color:%u), Esperando confirmacion...", receivedMessage.base.deviceID, receivedMessage.payload.trunk_protector.player_color);
            }

            if (receivedMessage.base.deviceType == CONTROL_TRANSMITTER) {
                bool directActionController = false;
                for (uint8_t i = 0; sizeof(directActions)/sizeof(directActions[0]); i++) {
                    if (receivedMessage.payload.control_transmitter.data == directActions[i]){
                        directActionController = true;
                        break;
                    }
                }
                if (directActionController) {
                    if (DEBUG_MODE) Serial.printf("[Accion Directa Control %u] Dato: %u", receivedMessage.base.deviceID, receivedMessage.payload.control_transmitter.data);
                    else Serial.printf("%u\n", receivedMessage.payload.control_transmitter.data);
                }
                else {
                    unsigned long timeDiff = millis() - firstMessageTime;
                    if (waitingForControllerConfirmation_blue && 
                        (receivedMessage.payload.control_transmitter.data == BLUE_PUNCH ||
                        receivedMessage.payload.control_transmitter.data == BLUE_BODY_KICK ||
                        receivedMessage.payload.control_transmitter.data == BLUE_BODY_TECHNICAL_KICK)) {
                        if (timeDiff <= MAX_WAIT_TIME) {
                            if (DEBUG_MODE) {
                                Serial.printf("¡Confirmacion! Acción: %u, Tiempo de diferencia: %ums\n", receivedMessage.payload.control_transmitter.data, timeDiff);
                            }
                            else Serial.printf("%u\n", receivedMessage.payload.control_transmitter.data);

                            turnOnPlayerLed(receivedMessage.payload.control_transmitter.data);
                        }
                    }
                    if (waitingForControllerConfirmation_red && 
                        (receivedMessage.payload.control_transmitter.data == RED_PUNCH ||
                        receivedMessage.payload.control_transmitter.data == RED_BODY_KICK ||
                        receivedMessage.payload.control_transmitter.data == RED_BODY_TECHNICAL_KICK)) {
                        if (timeDiff <= MAX_WAIT_TIME) {
                            if (DEBUG_MODE) {
                                Serial.printf("¡Confirmacion! Acción: %u, Tiempo de diferencia: %ums\n", receivedMessage.payload.control_transmitter.data, timeDiff);
                            }
                            else Serial.printf("%u\n", receivedMessage.payload.control_transmitter.data);

                            turnOnPlayerLed(receivedMessage.payload.control_transmitter.data);
                        }
                    }
                }
            }
            break;
        
        default:
            break;
    }
}

void setup() {
    Serial.begin(115200);
    Serial.println("\nInicializando Receptor...");
    Serial.print("MAC Receptor: ");
    Serial.println(WiFi.macAddress());

    // Declaramos los pines de salida y los apagamos
    pinMode(pinLedBlue, OUTPUT);
    pinMode(pinLedRed, OUTPUT);
    digitalWrite(pinLedBlue, LOW);
    digitalWrite(pinLedRed, LOW);
    
    dipswitch_config();
    dipValue = read_dipswitch();

    Serial.printf("Valor leído del DIP switch: 0x%02X\n", dipValue);

    // Configurar el modo de operacion basado en DIP switch
    switch (dipValue) {
        case 0x00:
            RECEIVING_DEVICE_MODE = ONE_CONTROL;
            DEBUG_MODE = false;
            break;
        case 0x01:
            RECEIVING_DEVICE_MODE = TWO_CONTROL;
            DEBUG_MODE = false;
            break;
        case 0x02:
            RECEIVING_DEVICE_MODE = JUST_TRUNK_PROTECTOR;
            DEBUG_MODE = false;
            break;
        case 0x03:
            RECEIVING_DEVICE_MODE = TRUNK_PROTECTOR_OR_CONTROL;
            DEBUG_MODE = false;
            break;
        case 0x04:
            RECEIVING_DEVICE_MODE = TRUNK_PROTECTOR_WITH_CONTROL_CONFIRMATION;
            DEBUG_MODE = false;
            break;
        case 0x08:
            RECEIVING_DEVICE_MODE = ONE_CONTROL;
            DEBUG_MODE = true;
            break;
        case 0x09:
            RECEIVING_DEVICE_MODE = TWO_CONTROL;
            DEBUG_MODE = true;
            break;
        case 0x0A:
            RECEIVING_DEVICE_MODE = JUST_TRUNK_PROTECTOR;
            DEBUG_MODE = true;
            break;
        case 0x0B:
            RECEIVING_DEVICE_MODE = TRUNK_PROTECTOR_OR_CONTROL;
            DEBUG_MODE = true;
            break;
        case 0x0C:
            RECEIVING_DEVICE_MODE = TRUNK_PROTECTOR_WITH_CONTROL_CONFIRMATION;
            DEBUG_MODE = true;
            break;
        default:
            RECEIVING_DEVICE_MODE = ONE_CONTROL;  // Modo por defecto
            DEBUG_MODE = true;
            Serial.println("Configuración DIP no reconocida. Usando modo por defecto (ONE CONTROL, DEBUG_MODE = true).");
            break;
    }

    espnow_init();
}

void loop() {
    turnOffPlayerLed();
    switch (RECEIVING_DEVICE_MODE) {
        case TWO_CONTROL:
            // Si pasó el tiempo máximo y no llegó el segundo mensaje, reiniciar
            if (waitingForSecondControllerConfirmation && (millis() - firstMessageTime > MAX_WAIT_TIME)) {
                if (DEBUG_MODE) Serial.println("Tiempo agotado. Ignorando primer dato.");

                waitingForSecondControllerConfirmation = false;
            }
            break;
        
        case TRUNK_PROTECTOR_WITH_CONTROL_CONFIRMATION:
            // Si pasó el tiempo máximo y no llegó el segundo mensaje, reiniciar
            if (waitingForControllerConfirmation_blue && (millis() - firstMessageTime > MAX_WAIT_TIME)) {
                if (DEBUG_MODE) Serial.println("Tiempo agotado (azul). Ignorando primer dato.");

                waitingForControllerConfirmation_blue = false;
            }
            if (waitingForControllerConfirmation_red && (millis() - firstMessageTime > MAX_WAIT_TIME)) {
                if (DEBUG_MODE) Serial.println("Tiempo agotado (rojo). Ignorando primer dato.");

                waitingForControllerConfirmation_red = false;
            }
            break;

        default:
            break;
    }
}

void espnow_init() {
    WiFi.mode(WIFI_STA);
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error al iniciar ESP-NOW");
        while (1); // Detener ejecución si hay error
    }
    esp_now_register_recv_cb(OnDataReceived);
    
    if (DEBUG_MODE) {
        Serial.println("ESP-NOW inicializado correctamente");
    }
}

void turnOnPlayerLed(uint8_t data) {
    if (data <= 5) {
        ledBlueTurnOnTime = millis();
        digitalWrite(pinLedBlue, HIGH);
    }
    else {
        ledRedTurnOnTime = millis();
        digitalWrite(pinLedRed, HIGH);
    }
}

void turnOffPlayerLed() {
    if (millis() - ledBlueTurnOnTime > MAX_WAIT_TIME_LED) {
        digitalWrite(pinLedBlue, LOW);
    }

    if (millis() - ledRedTurnOnTime > MAX_WAIT_TIME_LED) {
        digitalWrite(pinLedRed, LOW);
    }
}