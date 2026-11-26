#ifndef SENSOR_NODO_H
#define SENSOR_NODO_H

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

struct LecturaSensores {
  float temp;
  float pres;
  float hum;
  int gas;
};

class SensorNodo {
public:
  SensorNodo(const char* location, int pinMq135);
  bool begin();
  LecturaSensores leer();
  String formatearPayload(const LecturaSensores& datos) const;

private:
  const char* _location;
  int _pinMq135;
  Adafruit_BME280 _bme;
  bool _bmeOk;
};

#endif