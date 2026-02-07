#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <Preferences.h>

/* ===== PIN ===== */
#define LED_RED     25
#define LED_YELLOW  26
#define LED_GREEN   27
#define BTN_MANUAL  14

/* ===== WIFI ===== */
const char* ssid = "anif";
const char* pass = "12345688";

/* ===== MQTT ===== */
const char* mqtt_server = "df98e8e04d9c43d7a8eb03c565ab847a.s1.eu.hivemq.cloud";
const int   mqtt_port   = 8883;
const char* mqtt_topic  = "iot/esp32/status";
const char* mqtt_ctrl   = "iot/esp32/control";
const char* mqtt_user   = "hivemq.webclient.1770438253516";
const char* mqtt_pass   = "OP9S&z#6A>K3Gh4j;dyg";

/* ===== GLOBAL ===== */
Preferences prefs;
WiFiClientSecure net;
PubSubClient mqtt(net);

volatile bool manualPressed = false;
int trafficScore = 0;

/* ===== ISR ===== */
void IRAM_ATTR buttonISR() {
  manualPressed = true;
}

/* ===== PWM ===== */
void setPWM(uint8_t pin, uint8_t val) {
  ledcWrite(pin, val);
}

/* ===== AI LOGIC ===== */
int calculateGreenTime() {
  int base = 5000;
  int adaptive = trafficScore * 1000;
  return constrain(base + adaptive, 5000, 15000);
}

/* ===== MQTT CALLBACK (CONTROL) ===== */
void mqttCallback(char* topic, byte* payload, unsigned int length) {
  if (strcmp(topic, mqtt_ctrl) != 0) return;

  StaticJsonDocument<128> doc;
  if (deserializeJson(doc, payload, length)) return;

  if (doc.containsKey("reset")) {
    trafficScore = 0;
  }

  if (doc.containsKey("score")) {
    trafficScore = constrain(doc["score"], 0, 10);
  }

  prefs.putInt("traffic", trafficScore);

  Serial.println("[MQTT] CONTROL RECEIVED");
}

/* ===== MQTT SEND STATUS ===== */
void sendStatusJSON(int greenTime) {
  StaticJsonDocument<256> doc;
  char buffer[256];

  doc["device"]   = "ESP32-TRAFFIC-AI";
  doc["score"]    = trafficScore;
  doc["green_ms"] = greenTime;
  doc["green_s"]  = greenTime / 1000;
  doc["wifi"]     = (WiFi.status() == WL_CONNECTED) ? "ONLINE" : "OFFLINE";
  doc["uptime"]   = millis();

  serializeJson(doc, buffer);

  prefs.putString("last_json", buffer);

  if (mqtt.connected()) {
    mqtt.publish(mqtt_topic, buffer);
  }

  Serial.println(buffer);
}

/* ===== MQTT TASK ===== */
void mqttTask(void *pv) {
  for (;;) {
    if (WiFi.status() == WL_CONNECTED) {
      if (!mqtt.connected()) {
        if (mqtt.connect("ESP32_TRAFFIC_AI", mqtt_user, mqtt_pass)) {
          mqtt.subscribe(mqtt_ctrl);
          Serial.println("[MQTT] Connected");
        }
      }
      mqtt.loop();
    }
    vTaskDelay(200 / portTICK_PERIOD_MS);
  }
}

/* ===== TRAFFIC TASK ===== */
void trafficTask(void *pv) {
  while (1) {

    // RED
    setPWM(LED_RED, 255);
    setPWM(LED_YELLOW, 0);
    setPWM(LED_GREEN, 0);
    vTaskDelay(3000 / portTICK_PERIOD_MS);

    // YELLOW FADE
    setPWM(LED_RED, 0);
    for (int i = 0; i <= 255; i += 5) {
      setPWM(LED_YELLOW, i);
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    for (int i = 255; i >= 0; i -= 5) {
      setPWM(LED_YELLOW, i);
      vTaskDelay(20 / portTICK_PERIOD_MS);
    }
    setPWM(LED_YELLOW, 0);

    // GREEN (AI)
    int greenTime = calculateGreenTime();
    setPWM(LED_GREEN, 180);
    vTaskDelay(greenTime / portTICK_PERIOD_MS);
    setPWM(LED_GREEN, 0);

    // LEARNING
    if (manualPressed) {
      trafficScore++;
      manualPressed = false;
    } else {
      trafficScore--;
    }

    trafficScore = constrain(trafficScore, 0, 10);
    prefs.putInt("traffic", trafficScore);

    Serial.printf("[AI] Score=%d Green=%dms\n", trafficScore, greenTime);

    sendStatusJSON(greenTime);
  }
}

/* ===== WIFI TASK ===== */
void wifiTask(void *pv) {
  Serial.println("[WIFI] Connecting...");
  WiFi.begin(ssid, pass);

  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - t0 < 5000) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[WIFI] IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[WIFI] Offline mode");
  }

  vTaskDelete(NULL);
}

/* ===== SETUP ===== */
void setup() {
  Serial.begin(115200);

  ledcAttach(LED_RED,    5000, 8);
  ledcAttach(LED_YELLOW, 5000, 8);
  ledcAttach(LED_GREEN,  5000, 8);

  pinMode(BTN_MANUAL, INPUT_PULLUP);
  attachInterrupt(BTN_MANUAL, buttonISR, FALLING);

  prefs.begin("trafficAI", false);
  trafficScore = prefs.getInt("traffic", 0);

  net.setInsecure();
  mqtt.setServer(mqtt_server, mqtt_port);
  mqtt.setCallback(mqttCallback);

  Serial.println("[SYSTEM] READY");

  xTaskCreatePinnedToCore(trafficTask, "Traffic", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(wifiTask,    "WiFi",    4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(mqttTask,    "MQTT",    4096, NULL, 1, NULL, 0);
}

/* ===== LOOP ===== */
void loop() {
  vTaskDelay(1000 / portTICK_PERIOD_MS);
}
