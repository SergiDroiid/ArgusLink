#include "BotTelegram.h"
#include "Secretos.h"

// ============================================================
//  CONFIGURACIÓN DE INTERVALOS Y ALARMAS
// ============================================================

static const unsigned long REPORT_INTERVAL      = 30000UL;   // 30 s
static const unsigned long ALARM_CHECK_INTERVAL = 30000UL;   // cada 30 s
static const unsigned long ALARM_COOLDOWN       = 600000UL;  // 10 minutos

// ============================================================
//  ALARMAS — define aquí tus umbrales
//  superarUmbral = true  -> alerta si valor > umbral
//  superarUmbral = false -> alerta si valor < umbral
// ============================================================
static const DefAlarma ALARMAS[] = {
  { "temp_01",  25.0f, true,  "Temperatura ALTA"  },
  { "temp_01",  10.0f, false, "Temperatura BAJA"  },
  { "hum_01",   90.0f, true,  "Humedad ALTA"      },
  { "hum_01",   20.0f, false, "Humedad BAJA"      },
  { "gas_01",  700.0f, true,  "Calidad aire MALA" },
};
static const int N_ALARMAS = sizeof(ALARMAS) / sizeof(ALARMAS[0]);

BotTelegram bot(SECRET_WIFI_SSID, SECRET_WIFI_PASS,
                SECRET_FB_HOST, SECRET_FB_API_KEY,
                SECRET_FB_EMAIL, SECRET_FB_PASSWORD,
                SECRET_TG_TOKEN, SECRET_TG_CHAT_ID,
                ALARMAS, N_ALARMAS,
                REPORT_INTERVAL, ALARM_CHECK_INTERVAL, ALARM_COOLDOWN);

void setup() {
  Serial.begin(115200);
  delay(1000);
  Serial.println(F("\n=== Servidor Telegram ArgusLink ==="));
  bot.begin();
}

void loop() {
  bot.actualizar();
}