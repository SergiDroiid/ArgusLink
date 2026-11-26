#include "SensorNodo.h"
#include "EnlaceLoRa.h"

// NSS: 10, DIO0: 4, RST: 9, DIO1: 3
static const int PIN_LORA_NSS  = 10;
static const int PIN_LORA_DIO0 = 4;
static const int PIN_LORA_RST  = 9;
static const int PIN_LORA_DIO1 = 3;

static const float FRECUENCIA    = 868.0f;
static const byte SYNC_WORD      = 0x12;
static const int8_t POTENCIA_DBM = 17;

static const char LOCATION[]          = "Sala principal";
static const unsigned long INTERVALO_MS = 10000UL;
static const int PIN_MQ135            = A0;

SensorNodo sensores(LOCATION, PIN_MQ135);
EnlaceLoRa enlaceLoRa(PIN_LORA_NSS, PIN_LORA_DIO0, PIN_LORA_RST, PIN_LORA_DIO1,
                      FRECUENCIA, SYNC_WORD, POTENCIA_DBM);

unsigned long ultimoEnvio = 0;

void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println(F("\n--- INICIANDO EMISOR ---"));
  sensores.begin();

  if (!enlaceLoRa.begin()) {
    while (true);
  }
}

void loop() {
  if (millis() - ultimoEnvio < INTERVALO_MS) return;
  ultimoEnvio = millis();

  Serial.println(F("\n--- Ciclo de Lectura ---"));
  LecturaSensores datos = sensores.leer();
  String mensaje = sensores.formatearPayload(datos);
  enlaceLoRa.enviar(mensaje);
}