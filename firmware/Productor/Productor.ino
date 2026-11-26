#include "ReceptorLoRa.h"
#include "SubidaFirebase.h"
#include "Secretos.h"

static const int PIN_LORA_NSS  = 10;
static const int PIN_LORA_DIO0 = 4;
static const int PIN_LORA_RST  = 9;
static const int PIN_LORA_DIO1 = 3;

static const float FRECUENCIA = 868.0f;
static const byte SYNC_WORD   = 0x12;

ReceptorLoRa receptor(PIN_LORA_NSS, PIN_LORA_DIO0, PIN_LORA_RST, PIN_LORA_DIO1,
                      FRECUENCIA, SYNC_WORD);
SubidaFirebase firebase(SECRET_WIFI_SSID, SECRET_WIFI_PASS,
                        SECRET_FB_HOST, SECRET_FB_API_KEY,
                        SECRET_FB_EMAIL, SECRET_FB_PASSWORD);

void setup() {
  Serial.begin(115200);
  while (!Serial);
  Serial.println(F("\n=== RECEPTOR R4 INICIADO ==="));

  if (!receptor.begin()) {
    while (true);
  }
}

void loop() {
  String mensaje;
  if (receptor.recibir(mensaje)) {
    Serial.println(F("\n[LoRa] Mensaje recibido!"));
    receptor.dormir();
    delay(200);

    PaqueteSensor pkt = ReceptorLoRa::parsear(mensaje);
    if (pkt.valido) {
      firebase.subirLecturaCompleta(pkt);
    }

    Serial.println(F("--- Regresando a LoRa ---"));
    receptor.iniciarRecepcion();
  }
}