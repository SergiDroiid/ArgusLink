#ifndef ENLACE_LORA_H
#define ENLACE_LORA_H

#include <Arduino.h>
#include <RadioLib.h>

class EnlaceLoRa {
public:
  EnlaceLoRa(int nss, int dio0, int rst, int dio1, float frecuencia, byte syncWord, int8_t potencia);
  bool begin();
  bool enviar(String& mensaje);

private:
  SX1276 _radio;
  float _frecuencia;
  byte _syncWord;
  int8_t _potencia;
};

#endif