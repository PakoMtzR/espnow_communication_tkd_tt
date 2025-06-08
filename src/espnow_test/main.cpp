#include <esp_now.h>
#include <WiFi.h>

void setup() {
    Serial.begin(115200);
    Serial.println("Iniciando prueba mínima ESP-NOW");
    
    WiFi.mode(WIFI_STA);
    Serial.print("Direccion MAC: ");
    Serial.println(WiFi.macAddress());
    
    if (esp_now_init() != ESP_OK) {
        Serial.println("Error inicializando ESP-NOW");
        return;
    }
    Serial.println("ESP-NOW inicializado correctamente");
}

void loop() {
    delay(1000);
}