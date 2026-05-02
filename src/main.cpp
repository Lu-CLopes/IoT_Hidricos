#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

const float STEP = ((PH_V9 * 1.5f) - (PH_V7 * 1.5f)) / (7.0f - PH_PHn);

Adafruit_ADS1115 ads;
WiFiClient wifiClient;
PubSubClient mqtt(wifiClient);

struct SensorAccumulator
{
    double sumTDS = 0;
    double sumTurbidity = 0;
    double sumPH = 0;
    uint32_t count = 0;
};

SensorAccumulator acc;
unsigned long collectionStart = 0;

// ── Protótipos ────────────────────────────────────────────────────────────────
void connectWiFi();
void connectMQTT();
void publishAverages(float tds, float turbidity, float ph);
float readTDS(float v);
float readTurbidity(float v);
float readPH(float v);

// ── Setup ─────────────────────────────────────────────────────────────────────
void setup()
{
    Serial.begin(115200);
    delay(2000);

    Wire.begin();
    if (!ads.begin())
    {
        Serial.println("[ERRO] ADS1115 nao encontrado!");
        while (1)
            delay(1000);
    }
    ads.setGain(GAIN_ONE);

    connectWiFi();

    mqtt.setServer(TB_HOST, TB_PORT);
    mqtt.setSocketTimeout(10);
    mqtt.setBufferSize(512);

    collectionStart = millis();
    Serial.println("[OK] Iniciado. Coletando amostras...");
}

// ── Loop ──────────────────────────────────────────────────────────────────────
void loop()
{
    if (!mqtt.connected())
        connectMQTT();
    mqtt.loop();

    float vEC = ads.computeVolts(ads.readADC_SingleEnded(0));
    float vTurb = ads.computeVolts(ads.readADC_SingleEnded(1));
    float vPH = ads.computeVolts(ads.readADC_SingleEnded(2));

    float tds = readTDS(vEC);
    float turbidity = readTurbidity(vTurb);
    float ph = readPH(vPH);

    acc.sumTDS += tds;
    acc.sumTurbidity += turbidity;
    acc.sumPH += ph;
    acc.count++;

    Serial.printf("[%lums] TDS=%.0f ppm | Turb=%.0f NTU | pH=%.3f\n",
                  millis() - collectionStart, tds, turbidity, ph);

    if (millis() - collectionStart >= COLLECTION_PERIOD_MS && acc.count > 0)
    {
        float avgTDS = acc.sumTDS / acc.count;
        float avgTurbidity = acc.sumTurbidity / acc.count;
        float avgPH = acc.sumPH / acc.count;

        Serial.printf("\n=== MEDIAS (%lu amostras) ===\n", acc.count);
        Serial.printf("  TDS:      %.1f ppm\n", avgTDS);
        Serial.printf("  Turbidez: %.1f NTU\n", avgTurbidity);
        Serial.printf("  pH:       %.3f\n\n", avgPH);

        publishAverages(avgTDS, avgTurbidity, avgPH);

        acc = SensorAccumulator{};
        collectionStart = millis();
    }

    delay(SAMPLE_INTERVAL_MS);
}

// ── Conversões ────────────────────────────────────────────────────────────────
float readTDS(float v)
{
    return v * 1000.0f / 2.3f;
}

float readTurbidity(float v)
{
    return v * 3000.0f / 3.3f;
}

float readPH(float v)
{
    return 7.0f - (((v * 1.5f) - (PH_V7 * 1.5f)) / STEP);
}

// ── Wi-Fi ─────────────────────────────────────────────────────────────────────
void connectWiFi()
{
    Serial.printf("[WiFi] Conectando a %s", WIFI_SSID);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n[WiFi] Conectado! IP: %s\n",
                  WiFi.localIP().toString().c_str());
}

// ── MQTT ──────────────────────────────────────────────────────────────────────
void connectMQTT()
{
    while (!mqtt.connected())
    {
        Serial.print("[MQTT] Conectando ao ThingsBoard...");
        if (mqtt.connect("esp32-water-monitor", TB_TOKEN, ""))
        {
            Serial.println(" conectado!");
        }
        else
        {
            Serial.printf(" falhou (rc=%d). Tentando em 5s...\n", mqtt.state());
            delay(5000);
        }
    }
}

void publishAverages(float tds, float turbidity, float ph)
{
    if (!mqtt.connected())
        return;

    JsonDocument doc;
    doc["tds_ppm"] = round(tds * 10) / 10.0;
    doc["turbidity_ntu"] = round(turbidity * 10) / 10.0;
    doc["ph"] = round(ph * 1000) / 1000.0;
    doc["samples"] = acc.count;

    char payload[256];
    serializeJson(doc, payload);

    bool ok = mqtt.publish("v1/devices/me/telemetry", payload);
    Serial.printf("[MQTT] %s -> %s\n", ok ? "OK" : "FALHOU", payload);
}