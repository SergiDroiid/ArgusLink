#ifndef RECEPTOR_LORA_H
#define RECEPTOR_LORA_H

#include <Arduino.h>
#include <RadioLib.h>

struct PaqueteSensor {
  bool valido;
  String location;
  float temp;
  float pres;
  float hum;
  float gas;
};

class ReceptorLoRa {
public:
  ReceptorLoRa(int nss, int dio0, int rst, int dio1, float frecuencia, byte syncWord);
  bool begin();
  bool recibir(String& mensajeOut);
  void dormir();
  void iniciarRecepcion();
  static PaqueteSensor parsear(const String& raw);

private:
  SX1276 _radio;
  float _frecuencia;
  byte _syncWord;
};

#endif