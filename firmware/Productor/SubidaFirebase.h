#ifndef SUBIDA_FIREBASE_H
#define SUBIDA_FIREBASE_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include "ReceptorLoRa.h"

class SubidaFirebase {
public:
  SubidaFirebase(const char* wifiSsid, const char* wifiPass,
                 const char* fbHost, const char* fbApiKey,
                 const char* fbEmail, const char* fbPassword);
  bool subirLecturaCompleta(const PaqueteSensor& pkt);

private:
  const char* _wifiSsid;
  const char* _wifiPass;
  const char* _fbHost;
  const char* _fbApiKey;
  const char* _fbEmail;
  const char* _fbPassword;

  WiFiSSLClient _sslClient;
  String _idToken;
  unsigned long _tokenExpiry;

  static String extraerCampoJson(const String& json, const String& key);
  bool conectarWiFi();
  bool autenticar();
  bool enviarValor(const String& sensorId, float valor, const String& location);
};

#endif