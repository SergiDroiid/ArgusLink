#ifndef BOT_TELEGRAM_H
#define BOT_TELEGRAM_H

#include <Arduino.h>
#include <WiFiS3.h>
#include <ArduinoHttpClient.h>
#include <ArduinoJson.h>

struct DefAlarma {
  const char* sensorId;
  float       umbral;
  bool        superarUmbral;
  const char* descripcion;
};

class BotTelegram {
public:
  BotTelegram(const char* wifiSsid, const char* wifiPass,
              const char* fbHost, const char* fbApiKey,
              const char* fbEmail, const char* fbPassword,
              const char* tgToken, const char* tgChatId,
              const DefAlarma* alarmas, int numAlarmas,
              unsigned long reportInterval,
              unsigned long alarmCheckInterval,
              unsigned long alarmCooldown);
  ~BotTelegram();

  void begin();
  void actualizar();

private:
  const char* _wifiSsid;
  const char* _wifiPass;
  const char* _fbHost;
  const char* _fbApiKey;
  const char* _fbEmail;
  const char* _fbPassword;
  const char* _tgToken;
  const char* _tgChatId;

  const DefAlarma* _alarmas;
  int _numAlarmas;
  unsigned long* _alarmaUltimaAlerta;

  unsigned long _reportInterval;
  unsigned long _alarmCheckInterval;
  unsigned long _alarmCooldown;

  String _fbIdToken;
  String _fbRefreshToken;
  unsigned long _tokenExpiry;

  unsigned long _ultimoReport;
  unsigned long _ultimoAlarmCheck;

  static String authField(const String& json, const String& key);
  static String iconoSensor(const String& sid, const String& name);
  static String escaparHtml(const String& texto);
  static String formatTs(long long tsMs);

  bool signIn();
  bool renovarToken();
  bool ensureToken();
  String fbGet(const String& path);
  bool tgSend(const String& texto);
  bool leerSensores(String& mensajeOut);
  void comprobarAlarmas();
  bool verificarWiFi();
};

#endif