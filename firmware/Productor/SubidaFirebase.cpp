#include "SubidaFirebase.h"

SubidaFirebase::SubidaFirebase(const char* wifiSsid, const char* wifiPass,
                               const char* fbHost, const char* fbApiKey,
                               const char* fbEmail, const char* fbPassword)
  : _wifiSsid(wifiSsid),
    _wifiPass(wifiPass),
    _fbHost(fbHost),
    _fbApiKey(fbApiKey),
    _fbEmail(fbEmail),
    _fbPassword(fbPassword),
    _idToken(""),
    _tokenExpiry(0) {}

String SubidaFirebase::extraerCampoJson(const String& json, const String& key) {
  String needle = "\"" + key + "\":";
  int i = json.indexOf(needle);
  if (i < 0) return "";
  i += needle.length();
  while (i < (int)json.length() && json[i] == ' ') i++;
  if (json[i] != '"') return "";
  i++;
  int j = json.indexOf('"', i);
  return (j < 0) ? "" : json.substring(i, j);
}

bool SubidaFirebase::conectarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;

  Serial.print(F("Conectando WiFi..."));
  WiFi.begin(_wifiSsid, _wifiPass);
  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - start < 15000) {
    delay(500);
    Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F(" ¡OK!"));
    return true;
  }
  Serial.println(F(" FALLO"));
  return false;
}

bool SubidaFirebase::autenticar() {
  for (int intento = 1; intento <= 3; intento++) {
    Serial.print(F("[Auth] Intento "));
    Serial.print(intento);
    Serial.print(F("... "));

    _sslClient.stop();
    HttpClient http(_sslClient, "identitytoolkit.googleapis.com", 443);
    http.setHttpResponseTimeout(8000);

    String path = "/v1/accounts:signInWithPassword?key=" + String(_fbApiKey);
    String body = "{\"email\":\"" + String(_fbEmail) +
                  "\",\"password\":\"" + String(_fbPassword) +
                  "\",\"returnSecureToken\":true}";

    http.post(path, "application/json", body);
    int status = http.responseStatusCode();
    String resp = http.responseBody();

    if (status == 200) {
      _idToken = extraerCampoJson(resp, "idToken");
      _tokenExpiry = millis() + (extraerCampoJson(resp, "expiresIn").toInt() - 60) * 1000UL;
      Serial.println(F("OK"));
      return true;
    }
    Serial.print(F("Error "));
    Serial.println(status);
    delay(2000);
  }
  return false;
}

bool SubidaFirebase::enviarValor(const String& sensorId, float valor, const String& location) {
  HttpClient fb(_sslClient, _fbHost, 443);
  fb.setHttpResponseTimeout(5000);

  String path = "/readings/" + sensorId + ".json?auth=" + _idToken;
  String body = "{\"value\":" + String(valor, 2) +
                ",\"ts\":{\".sv\":\"timestamp\"},\"location\":\"" + location + "\"}";

  Serial.print(F("Subiendo "));
  Serial.print(sensorId);
  Serial.print(F("... "));
  fb.post(path, "application/json", body);
  int status = fb.responseStatusCode();
  fb.responseBody();

  if (status == 200) {
    Serial.println(F("OK"));
    return true;
  }
  Serial.print(F("Fallo "));
  Serial.println(status);
  return false;
}

bool SubidaFirebase::subirLecturaCompleta(const PaqueteSensor& pkt) {
  if (!pkt.valido) return false;
  if (!conectarWiFi()) return false;

  if (_idToken.isEmpty() || millis() > _tokenExpiry) {
    if (!autenticar()) {
      Serial.println(F("Abortando por falta de Auth"));
      WiFi.disconnect();
      return false;
    }
  }

  enviarValor("temp_01", pkt.temp, pkt.location);
  delay(300);
  enviarValor("pres_01", pkt.pres, pkt.location);
  delay(300);
  enviarValor("hum_01", pkt.hum, pkt.location);
  delay(300);
  enviarValor("gas_01", pkt.gas, pkt.location);

  Serial.println(F(">>> CICLO COMPLETADO"));
  delay(500);
  return true;
}