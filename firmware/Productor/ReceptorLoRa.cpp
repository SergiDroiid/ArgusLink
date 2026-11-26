#include "ReceptorLoRa.h"

ReceptorLoRa::ReceptorLoRa(int nss, int dio0, int rst, int dio1, float frecuencia, byte syncWord)
  : _radio(new Module(nss, dio0, rst, dio1)),
    _frecuencia(frecuencia),
    _syncWord(syncWord) {}

bool ReceptorLoRa::begin() {
  int state = _radio.begin(_frecuencia);
  if (state == RADIOLIB_ERR_NONE) {
    _radio.setSyncWord(_syncWord);
    _radio.setBandwidth(125.0);
    _radio.setSpreadingFactor(9);
    _radio.setCodingRate(7);
    Serial.println(F("LoRa: LISTO"));
    return true;
  }
  Serial.print(F("Error LoRa: "));
  Serial.println(state);
  return false;
}

bool ReceptorLoRa::recibir(String& mensajeOut) {
  int state = _radio.receive(mensajeOut);
  if (state == RADIOLIB_ERR_NONE) {
    return true;
  }
  if (state != RADIOLIB_ERR_RX_TIMEOUT) {
    Serial.print(F("[LoRa] Error receive(), código: "));
    Serial.println(state);
  }
  return false;
}

void ReceptorLoRa::dormir() {
  _radio.sleep();
}

void ReceptorLoRa::iniciarRecepcion() {
  _radio.startReceive();
}

PaqueteSensor ReceptorLoRa::parsear(const String& raw) {
  PaqueteSensor pkt = {false, "", 0.0f, 0.0f, 0.0f, 0.0f};

  int p1 = raw.indexOf('|');
  int p2 = raw.indexOf('|', p1 + 1);
  int p3 = raw.indexOf('|', p2 + 1);
  int p4 = raw.indexOf('|', p3 + 1);
  if (p4 == -1) {
    Serial.println(F("Datos LoRa corruptos"));
    return pkt;
  }

  pkt.location = raw.substring(0, p1);
  pkt.temp = raw.substring(p1 + 1, p2).toFloat();
  pkt.pres = raw.substring(p2 + 1, p3).toFloat();
  pkt.hum = raw.substring(p3 + 1, p4).toFloat();
  pkt.gas = raw.substring(p4 + 1).toFloat();
  pkt.valido = true;
  return pkt;
}