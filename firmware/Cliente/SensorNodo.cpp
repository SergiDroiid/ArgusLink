#include "SensorNodo.h"

SensorNodo::SensorNodo(const char* location, int pinMq135)
  : _location(location), _pinMq135(pinMq135), _bmeOk(false) {}

bool SensorNodo::begin() {
  Wire.begin();
  pinMode(_pinMq135, INPUT);

  if (_bme.begin(0x76) || _bme.begin(0x77)) {
    Serial.println(F("[Sensor] BME280 detectado."));
    _bmeOk = true;
  } else {
    Serial.println(F("[Sensor] ERROR: No se encuentra BME280."));
    _bmeOk = false;
  }
  return _bmeOk;
}

LecturaSensores SensorNodo::leer() {
  LecturaSensores datos = {0.0f, 0.0f, 0.0f, 0};

  if (_bmeOk) {
    datos.temp = _bme.readTemperature();
    datos.pres = _bme.readPressure() / 100.0F;
    datos.hum = _bme.readHumidity();

    if (datos.temp > 80.0f || datos.temp < -20.0f) {
      Serial.println(F("[!] Lectura BME280 inválida, reintentando sensor..."));
      _bme.begin(0x76);
      datos.temp = 0.0f;
      datos.pres = 0.0f;
      datos.hum = 0.0f;
    }
  }

  datos.gas = analogRead(_pinMq135);
  return datos;
}

String SensorNodo::formatearPayload(const LecturaSensores& datos) const {
  return String(_location) + "|" +
         String(datos.temp, 1) + "|" +
         String(datos.pres, 1) + "|" +
         String(datos.hum, 1) + "|" +
         String(datos.gas);
}