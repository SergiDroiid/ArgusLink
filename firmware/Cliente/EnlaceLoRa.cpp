#include "EnlaceLoRa.h"

EnlaceLoRa::EnlaceLoRa(int nss, int dio0, int rst, int dio1, float frecuencia, byte syncWord, int8_t potencia)
  : _radio(new Module(nss, dio0, rst, dio1)),
    _frecuencia(frecuencia),
    _syncWord(syncWord),
    _potencia(potencia) {}

bool EnlaceLoRa::begin() {
  Serial.print(F("[LoRa] Configurando chip... "));
  int state = _radio.begin(_frecuencia);

  if (state == RADIOLIB_ERR_NONE) {
    _radio.setSyncWord(_syncWord);
    _radio.setBandwidth(125.0);
    _radio.setSpreadingFactor(9);
    _radio.setCodingRate(7);
    _radio.setOutputPower(_potencia);
    Serial.println(F("OK!"));
    return true;
  }

  Serial.print(F("Fallo crítico LoRa, código: "));
  Serial.println(state);
  return false;
}

bool EnlaceLoRa::enviar(String& mensaje) {
  Serial.print(F("[LoRa] Transmitiendo: "));
  Serial.println(mensaje);

  int state = _radio.transmit(mensaje);

  if (state == RADIOLIB_ERR_NONE) {
    Serial.println(F("[EXITO] Paquete enviado correctamente."));
    return true;
  }
  if (state == RADIOLIB_ERR_TX_TIMEOUT) {
    Serial.println(F("[ERROR] Timeout (Error -5). Revisa el pin DIO0 (Pin 4)."));
  } else {
    Serial.print(F("[ERROR] Fallo código: "));
    Serial.println(state);
  }
  return false;
}