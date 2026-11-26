#include "BotTelegram.h"

BotTelegram::BotTelegram(const char* wifiSsid, const char* wifiPass,
                       const char* fbHost, const char* fbApiKey,
                       const char* fbEmail, const char* fbPassword,
                       const char* tgToken, const char* tgChatId,
                       const DefAlarma* alarmas, int numAlarmas,
                       unsigned long reportInterval,
                       unsigned long alarmCheckInterval,
                       unsigned long alarmCooldown)
  : _wifiSsid(wifiSsid),
    _wifiPass(wifiPass),
    _fbHost(fbHost),
    _fbApiKey(fbApiKey),
    _fbEmail(fbEmail),
    _fbPassword(fbPassword),
    _tgToken(tgToken),
    _tgChatId(tgChatId),
    _alarmas(alarmas),
    _numAlarmas(numAlarmas),
    _alarmaUltimaAlerta(nullptr),
    _reportInterval(reportInterval),
    _alarmCheckInterval(alarmCheckInterval),
    _alarmCooldown(alarmCooldown),
    _fbIdToken(""),
    _fbRefreshToken(""),
    _tokenExpiry(0),
    _ultimoReport(0),
    _ultimoAlarmCheck(0) {
  if (_numAlarmas > 0) {
    _alarmaUltimaAlerta = new unsigned long[_numAlarmas]();
  }
}

BotTelegram::~BotTelegram() {
  delete[] _alarmaUltimaAlerta;
}

String BotTelegram::authField(const String& json, const String& key) {
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

bool BotTelegram::signIn() {
  Serial.print(F("[Auth] signIn... "));
  WiFiSSLClient ssl;
  HttpClient http(ssl, "identitytoolkit.googleapis.com", 443);
  http.setHttpResponseTimeout(10000);

  String path = "/v1/accounts:signInWithPassword?key=" + String(_fbApiKey);
  String body = "{\"email\":\""   + String(_fbEmail)    + "\","
                "\"password\":\"" + String(_fbPassword) + "\","
                "\"returnSecureToken\":true}";

  http.post(path, "application/json", body);
  int    status = http.responseStatusCode();
  String resp   = http.responseBody();
  http.stop();

  if (status != 200) {
    Serial.print(F("ERROR status=")); Serial.println(status);
    return false;
  }
  _fbIdToken      = authField(resp, "idToken");
  _fbRefreshToken = authField(resp, "refreshToken");
  String exp      = authField(resp, "expiresIn");
  _tokenExpiry    = millis() + (exp.toInt() - 60) * 1000UL;

  Serial.println(_fbIdToken.isEmpty() ? F("FALLO — token vacío") : F("OK"));
  return !_fbIdToken.isEmpty();
}

bool BotTelegram::renovarToken() {
  Serial.print(F("[Auth] refresh... "));
  WiFiSSLClient ssl;
  HttpClient http(ssl, "securetoken.googleapis.com", 443);
  http.setHttpResponseTimeout(10000);

  String path = "/v1/token?key=" + String(_fbApiKey);
  String body = "grant_type=refresh_token&refresh_token=" + _fbRefreshToken;

  http.post(path, "application/x-www-form-urlencoded", body);
  int    status = http.responseStatusCode();
  String resp   = http.responseBody();
  http.stop();

  if (status != 200) { Serial.println(F("ERROR")); return false; }
  _fbIdToken      = authField(resp, "id_token");
  _fbRefreshToken = authField(resp, "refresh_token");
  String exp      = authField(resp, "expires_in");
  _tokenExpiry    = millis() + (exp.toInt() - 60) * 1000UL;

  Serial.println(_fbIdToken.isEmpty() ? F("FALLO") : F("OK"));
  return !_fbIdToken.isEmpty();
}

bool BotTelegram::ensureToken() {
  if (_fbIdToken.isEmpty())      return signIn();
  if (millis() >= _tokenExpiry) return renovarToken();
  return true;
}

String BotTelegram::fbGet(const String& path) {
  WiFiSSLClient ssl;
  HttpClient fb(ssl, _fbHost, 443);
  fb.setHttpResponseTimeout(10000);
  String sep = (path.indexOf('?') >= 0) ? "&" : "?";
  fb.get(path + sep + "auth=" + _fbIdToken);
  int    status = fb.responseStatusCode();
  String body   = fb.responseBody();
  fb.stop();
  if (status != 200) {
    Serial.print(F("[fbGet] ERROR status=")); Serial.print(status);
    Serial.print(F(" path=")); Serial.println(path);
    return "null";
  }
  return body;
}

String BotTelegram::iconoSensor(const String& sid, const String& name) {
  String s = sid + " " + name;
  s.toLowerCase();
  if (s.indexOf("temp") >= 0) return "🌡️";
  if (s.indexOf("hum")  >= 0) return "💧";
  if (s.indexOf("gas")  >= 0 || s.indexOf("aire") >= 0 || s.indexOf("co2") >= 0) return "🌬️";
  if (s.indexOf("pres") >= 0) return "🧭";
  return "📡";
}

String BotTelegram::escaparHtml(const String& texto) {
  String out = texto;
  out.replace("&", "&amp;");
  out.replace("<", "&lt;");
  out.replace(">", "&gt;");
  out.replace("\"", "'");
  return out;
}

bool BotTelegram::tgSend(const String& texto) {
  Serial.print(F("[Telegram] enviando... "));

  WiFiSSLClient ssl;
  HttpClient tg(ssl, "api.telegram.org", 443);
  tg.setHttpResponseTimeout(10000);

  String path = "/bot";
  path += _tgToken;
  path += "/sendMessage";

  String textoEsc = texto;
  textoEsc.replace("\"", "'");

  String body = "{\"chat_id\":\"";
  body += _tgChatId;
  body += "\",\"text\":\"";
  body += textoEsc;
  body += "\",\"parse_mode\":\"HTML\"}";

  tg.post(path, "application/json", body);
  int    status = tg.responseStatusCode();
  String resp   = tg.responseBody();
  tg.stop();

  if (status != 200) {
    Serial.print(F("ERROR status=")); Serial.println(status);
    Serial.print(F("  resp: ")); Serial.println(resp.substring(0, 100));
    return false;
  }
  Serial.println(F("OK"));
  return true;
}

String BotTelegram::formatTs(long long tsMs) {
  if (tsMs <= 0) return "--";
  unsigned long ts = (unsigned long)(tsMs / 1000UL) + 3600UL;
  unsigned long t2 = ts % 86400UL;
  int hh = t2 / 3600, mm2 = (t2 % 3600) / 60;
  unsigned long days = ts / 86400UL;
  days += 719468;
  unsigned long era = days / 146097;
  unsigned long doe = days - era * 146097;
  unsigned long yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
  unsigned long doy = doe - (365*yoe + yoe/4 - yoe/100);
  unsigned long mp  = (5*doy + 2) / 153;
  int day   = doy - (153*mp + 2) / 5 + 1;
  int month = mp + (mp < 10 ? 3 : -9);
  int year  = (int)(yoe + era * 400) + (month <= 2 ? 1 : 0);
  char buf[20];
  snprintf(buf, sizeof(buf), "%02d/%02d/%04d %02d:%02d", day, month, year, hh, mm2);
  return String(buf);
}

bool BotTelegram::leerSensores(String& mensajeOut) {
  if (!ensureToken()) return false;

  String sensJson = fbGet("/sensors.json");
  if (sensJson == "null" || sensJson.length() < 3) {
    Serial.println(F("[Report] sin sensores"));
    return false;
  }

  JsonDocument sensDoc;
  if (deserializeJson(sensDoc, sensJson)) return false;
  JsonObject sensObj = sensDoc.as<JsonObject>();

  mensajeOut  = "━━━━━━━━━━━━━━━━━━━━\n";
  mensajeOut += "🛰️ <b>ARGUSLINK · ESTADO EN VIVO</b>\n";
  mensajeOut += "━━━━━━━━━━━━━━━━━━━━\n";

  for (JsonPair kv : sensObj) {
    String sid      = kv.key().c_str();
    JsonObject sv   = kv.value().as<JsonObject>();
    String name     = escaparHtml(sv["name"]     | sid);
    String unit     = escaparHtml(sv["unit"]     | "");
    String location = escaparHtml(sv["location"] | "");
    String ico      = iconoSensor(sid, name);

    mensajeOut += "\n" + ico + " <b>" + name + "</b>";
    if (location.length()) {
      mensajeOut += " · <i>" + location + "</i>";
    }
    mensajeOut += "\n";

    String rJson = fbGet("/readings/" + sid + ".json?orderBy=%22%24key%22&limitToLast=1");
    if (rJson == "null" || rJson.length() < 3) {
      mensajeOut += "   └ ⚪ <i>Sin lecturas registradas</i>\n";
      continue;
    }

    JsonDocument rDoc;
    if (deserializeJson(rDoc, rJson)) {
      mensajeOut += "   └ ⚠️ <i>Error al leer datos</i>\n";
      continue;
    }
    JsonObject rObj = rDoc.as<JsonObject>();

    for (JsonPair rk : rObj) {
      float     val = rk.value()["value"] | 0.0f;
      long long ts  = rk.value()["ts"]    | (long long)0;

      mensajeOut += "   ├ Lectura: <code>" + String(val, 2);
      if (unit.length()) mensajeOut += " " + unit;
      mensajeOut += "</code>\n";
      mensajeOut += "   └ 🕒 <code>" + formatTs(ts) + "</code>\n";
    }
  }

  mensajeOut += "\n━━━━━━━━━━━━━━━━━━━━\n";
  mensajeOut += "⏱️ <i>Próximo reporte en " + String(_reportInterval / 1000) + " s</i>";

  return true;
}

void BotTelegram::comprobarAlarmas() {
  if (!ensureToken()) return;

  unsigned long ahora = millis();

  for (int a = 0; a < _numAlarmas; a++) {
    if (_alarmaUltimaAlerta[a] != 0 && ahora - _alarmaUltimaAlerta[a] < _alarmCooldown) continue;

    String sid   = String(_alarmas[a].sensorId);
    String rJson = fbGet("/readings/" + sid + ".json?orderBy=%22%24key%22&limitToLast=1");

    if (rJson == "null" || rJson.length() < 3) continue;

    JsonDocument rDoc;
    if (deserializeJson(rDoc, rJson)) continue;
    JsonObject rObj = rDoc.as<JsonObject>();

    for (JsonPair rk : rObj) {
      float val = rk.value()["value"] | 0.0f;
      bool  disparar = _alarmas[a].superarUmbral ? (val > _alarmas[a].umbral)
                                                  : (val < _alarmas[a].umbral);
      if (!disparar) continue;

      String nombre = sid;
      String unidad = "";
      String ubicacion = "";
      String sJson  = fbGet("/sensors/" + sid + ".json");
      if (sJson != "null" && sJson.length() > 2) {
        JsonDocument sDoc;
        if (!deserializeJson(sDoc, sJson)) {
          nombre    = escaparHtml(sDoc["name"]     | sid);
          unidad    = escaparHtml(sDoc["unit"]     | "");
          ubicacion = escaparHtml(sDoc["location"] | "");
        }
      }

      String ico = iconoSensor(sid, nombre);
      String tipoUmbral = _alarmas[a].superarUmbral ? "Superado máximo" : "Por debajo del mínimo";

      String msg  = "🚨🚨🚨 <b>ALERTA ARGUSLINK</b> 🚨🚨🚨\n";
      msg += "━━━━━━━━━━━━━━━━━━━━\n";
      msg += "⚠️ <b>" + escaparHtml(_alarmas[a].descripcion) + "</b>\n\n";
      msg += ico + " <b>Sensor:</b> " + nombre + "\n";
      if (ubicacion.length()) {
        msg += "📍 <b>Zona:</b> " + ubicacion + "\n";
      }
      msg += "📌 <b>Valor actual:</b> <code>" + String(val, 2);
      if (unidad.length()) msg += " " + unidad;
      msg += "</code>\n";
      msg += "🎯 <b>Umbral:</b> <code>" + String(_alarmas[a].umbral, 2);
      if (unidad.length()) msg += " " + unidad;
      msg += "</code> (" + tipoUmbral + ")\n";
      msg += "🕒 <b>Hora:</b> <code>" + formatTs(rk.value()["ts"] | (long long)0) + "</code>\n";
      msg += "━━━━━━━━━━━━━━━━━━━━";

      Serial.print(F("[Alarma] ")); Serial.println(_alarmas[a].descripcion);
      if (tgSend(msg)) {
        _alarmaUltimaAlerta[a] = ahora;
      }
    }
  }
}

bool BotTelegram::verificarWiFi() {
  if (WiFi.status() == WL_CONNECTED) return true;
  Serial.print(F("WiFi caído — reconectando"));
  WiFi.begin(_wifiSsid, _wifiPass);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 20000) {
    delay(500); Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F(" OK"));
    delay(2000);
    return true;
  }
  Serial.println(F(" FALLO — se reintentará en el siguiente ciclo"));
  return false;
}

void BotTelegram::begin() {
  Serial.print(F("Conectando a '")); Serial.print(_wifiSsid); Serial.print(F("'"));
  WiFi.begin(_wifiSsid, _wifiPass);
  unsigned long t = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t < 20000) {
    delay(500); Serial.print('.');
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println(F(" OK"));
    Serial.print(F("  IP: ")); Serial.println(WiFi.localIP());
    delay(2000);
  } else {
    Serial.println(F(" FALLO — se reintentará en loop()"));
    return;
  }

  for (int i = 1; i <= 3 && _fbIdToken.isEmpty(); i++) {
    if (i > 1) delay(3000);
    signIn();
  }
  if (_fbIdToken.isEmpty()) {
    Serial.println(F("AVISO: sin token Firebase — se reintentará en el primer reporte"));
  }

  String inicioMsg  = "🟢 <b>ARGUSLINK BOT ACTIVO</b>\n";
  inicioMsg += "━━━━━━━━━━━━━━━━━━━━\n";
  inicioMsg += "📡 <b>Red:</b> " + escaparHtml(String(_wifiSsid)) + "\n";
  inicioMsg += "🔄 <b>Reporte cada:</b> <code>" + String(_reportInterval / 1000) + " s</code>\n";
  inicioMsg += "🔔 <b>Alarmas configuradas:</b> <code>" + String(_numAlarmas) + "</code>\n";
  inicioMsg += "━━━━━━━━━━━━━━━━━━━━";
  tgSend(inicioMsg);

  _ultimoReport     = millis() - _reportInterval;
  _ultimoAlarmCheck = millis() - _alarmCheckInterval;
}

void BotTelegram::actualizar() {
  if (!verificarWiFi()) {
    delay(5000);
    return;
  }

  unsigned long ahora = millis();

  if (ahora - _ultimoReport >= _reportInterval) {
    _ultimoReport = ahora;
    Serial.println(F("\n--- Reporte periódico ---"));
    String msg;
    if (leerSensores(msg)) {
      tgSend(msg);
    } else {
      Serial.println(F("  Sin datos para reportar"));
    }
  }

  if (ahora - _ultimoAlarmCheck >= _alarmCheckInterval) {
    _ultimoAlarmCheck = ahora;
    Serial.println(F("\n--- Comprobando alarmas ---"));
    comprobarAlarmas();
  }
}