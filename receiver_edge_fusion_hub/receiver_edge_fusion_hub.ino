#define BLYNK_TEMPLATE_ID   "TMPL3Z7Bps02h"
#define BLYNK_TEMPLATE_NAME "Disaster Monitor"
#define BLYNK_AUTH_TOKEN    "VWF649udruneNhe9b_WgSBEQdhONiFUS"

#include <WiFi.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include <BlynkSimpleEsp32.h>

// -------- LoRa Pins --------
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  27

// -------- WiFi --------
const char* ssid     = "priyanshu";
const char* password = "123456789";

// -------- Blynk VPins --------
#define VPIN_WATER_LEVEL    V0
#define VPIN_FLOOD_TIER     V1
#define VPIN_FLOOD_CONF     V2
#define VPIN_TEMP           V3
#define VPIN_HUMIDITY       V4
#define VPIN_SOIL           V5
#define VPIN_DROUGHT_RISK   V6
#define VPIN_DROUGHT_CONF   V7
#define VPIN_COMBINED_ALERT V8
#define VPIN_WATER_TREND    V9
#define VPIN_DRY_DAYS       V10
#define VPIN_RAINFALL       V11   // NEW — rainfall status string
#define VPIN_RISE_RATE      V12   // NEW — rise rate number

// -------- Node-1 Data --------
float  latest_water_level        = 0;
String latest_flood_tier         = "UNKNOWN";
float  latest_flood_confidence   = 0;
String latest_rainfall           = "NO_RAIN";   // NEW
float  latest_rise_rate          = 0;           // NEW

// -------- Node-2 Data --------
float  latest_temp               = 0;
float  latest_humidity           = 0;
int    latest_soil               = 0;
int    latest_dry_days           = 0;
String latest_drought_risk       = "no_drought";
float  latest_drought_confidence = 0;

// -------- History --------
float water_levels[10] = {0};
int   history_index    = 0;

// -------- Counters --------
int node1_count = 0;
int node2_count = 0;

BlynkTimer timer;

// ============================================================
// Helpers
// ============================================================
void updateWaterHistory(float level) {
  water_levels[history_index] = level;
  history_index = (history_index + 1) % 10;
}

float calculateTrend(float* data, int size) {
  float sum = 0; int count = 0;
  for (int i = 0; i < size; i++) {
    if (data[i] > 0) { sum += data[i]; count++; }
  }
  return (count > 0) ? (sum / count) : 0;
}

String combinedAlert() {
  float water_trend = calculateTrend(water_levels, 10);
  int score = 0;
  if (latest_soil < 25)          score += 35;
  if (latest_temp > 30)          score += 25;
  if (water_trend < 40)          score += 25;
  if (latest_dry_days > 5)       score += 15;
  if (latest_rainfall == "HEAVY_RAIN")    score += 20;
  if (latest_rainfall == "MODERATE_RAIN") score += 10;
  if (score >= 60) return "HIGH";
  if (score >= 30) return "MEDIUM";
  return "LOW";
}

// ============================================================
// Print Rainfall Alert on receiver serial
// ============================================================
void printRainfallAlert() {
  if (latest_rainfall == "HEAVY_RAIN") {
    Serial.println("┌──────────────────────────────────┐");
    Serial.println("│ 🚨 HEAVY RAINFALL DETECTED!      │");
    Serial.print  ("│    Rise rate: +");
    Serial.print  (latest_rise_rate, 1);
    Serial.println("% per 30s       │");
    Serial.println("│    Flood risk is HIGH!           │");
    Serial.println("└──────────────────────────────────┘");
  } else if (latest_rainfall == "MODERATE_RAIN") {
    Serial.println("⚠️  MODERATE RAIN — monitor closely");
  } else if (latest_rainfall == "LIGHT_RAIN") {
    Serial.println("🌧️  Light rain detected");
  } else if (latest_rainfall == "DRAINING") {
    Serial.println("↓  Water level falling");
  } else {
    Serial.println("✓  No rainfall");
  }
}

// ============================================================
// Print Full Status
// ============================================================
void printStatus() {
  Serial.println("\n========== DISASTER STATUS ==========");
  Serial.print("Node-1 packets: "); Serial.println(node1_count);
  Serial.print("Node-2 packets: "); Serial.println(node2_count);

  Serial.println("\n--- FLOOD (Node-1) ---");
  Serial.print("Water Level : "); Serial.print(latest_water_level); Serial.println("%");
  Serial.print("Flood Tier  : "); Serial.println(latest_flood_tier);
  Serial.print("Rainfall    : "); Serial.println(latest_rainfall);
  Serial.print("Rise Rate   : "); Serial.print(latest_rise_rate, 1); Serial.println("% per 30s");

  Serial.println("\n--- DROUGHT (Node-2) ---");
  Serial.print("Temperature : "); Serial.print(latest_temp); Serial.println("°C");
  Serial.print("Humidity    : "); Serial.print(latest_humidity); Serial.println("%");
  Serial.print("Soil        : "); Serial.print(latest_soil); Serial.println("%");
  Serial.print("Drought Risk: "); Serial.println(latest_drought_risk);

  Serial.println("\n--- RAINFALL ALERT ---");
  printRainfallAlert();
  Serial.println("=====================================\n");
}

// ============================================================
// Send to Blynk
// ============================================================
void sendToBlynk() {
  if (!Blynk.connected()) {
    Serial.println("⚠️  Blynk not connected");
    return;
  }

  // Flood data
  Blynk.virtualWrite(VPIN_WATER_LEVEL, latest_water_level);
  Blynk.virtualWrite(VPIN_FLOOD_TIER,  latest_flood_tier);
  Blynk.virtualWrite(VPIN_FLOOD_CONF,  latest_flood_confidence);

  // Rainfall data — NEW
  Blynk.virtualWrite(VPIN_RAINFALL,    latest_rainfall);
  Blynk.virtualWrite(VPIN_RISE_RATE,   latest_rise_rate);

  // Drought data
  Blynk.virtualWrite(VPIN_TEMP,         latest_temp);
  Blynk.virtualWrite(VPIN_HUMIDITY,     latest_humidity);
  Blynk.virtualWrite(VPIN_SOIL,         latest_soil);
  Blynk.virtualWrite(VPIN_DRY_DAYS,     latest_dry_days);
  Blynk.virtualWrite(VPIN_DROUGHT_RISK, latest_drought_risk);
  Blynk.virtualWrite(VPIN_DROUGHT_CONF, latest_drought_confidence);

  // Trend and combined
  float water_trend = calculateTrend(water_levels, 10);
  Blynk.virtualWrite(VPIN_WATER_TREND,    water_trend);
  Blynk.virtualWrite(VPIN_COMBINED_ALERT, combinedAlert());

  Serial.println("✓ Blynk updated");
}

// ============================================================
// Handle Incoming LoRa Packet
// ============================================================
void handlePacket() {
  String received = "";
  while (LoRa.available()) {
    received += (char)LoRa.read();
  }

  int rssi = LoRa.packetRssi();

  Serial.println("\n─── PACKET RECEIVED ───");
  Serial.print("RSSI : "); Serial.print(rssi); Serial.println(" dBm");
  Serial.print("Data : "); Serial.println(received);

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, received);

  if (error) {
    Serial.print("❌ JSON error: ");
    Serial.println(error.c_str());
    return;
  }

  String node_id = doc["node"] | "UNKNOWN";
  node_id.trim();

  if (node_id == "NODE-1") {
    node1_count++;
    latest_water_level      = doc["water_level_pct"]  | 0.0f;
    latest_flood_tier       = doc["tier"]              | "UNKNOWN";
    latest_flood_confidence = doc["confidence"]        | 0.0f;
    latest_rainfall         = doc["rainfall"]          | "NO_RAIN";
    latest_rise_rate        = doc["rise_rate"]         | 0.0f;
    updateWaterHistory(latest_water_level);
    Serial.println("✓ Node-1 saved");

  } else if (node_id == "NODE-2") {
    node2_count++;
    latest_temp               = doc["T2M"]           | 0.0f;
    latest_humidity           = doc["humidity"]       | 0.0f;
    latest_soil               = doc["soil_moisture"]  | 0;
    latest_dry_days           = doc["dry_days"]       | 0;
    latest_drought_risk       = doc["drought_risk"]   | "no_drought";
    latest_drought_confidence = doc["confidence"]     | 0.0f;
    Serial.println("✓ Node-2 saved");

  } else {
    Serial.println("⚠️  Unknown node: " + node_id);
  }

  printStatus();
}

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  RECEIVER — FLOOD + RAINFALL MONITOR  ║");
  Serial.println("╚════════════════════════════════════════╝");

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(865E6)) {
    Serial.println("❌ LoRa init failed");
    while (1) { delay(100); }
  }

  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSpreadingFactor(7);
  LoRa.enableCrc();

  Serial.println("✓ LoRa ready at 865 MHz");

  Serial.print("Connecting WiFi");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000); Serial.print(".");
  }
  Serial.println("\n✓ WiFi connected");

  Blynk.begin(BLYNK_AUTH_TOKEN, ssid, password);
  timer.setInterval(5000L, sendToBlynk);

  Serial.println("✓ Blynk connected");
  Serial.println("\n⏳ Listening for Node-1 and Node-2...\n");
}

// ============================================================
// Loop — NO delay here
// ============================================================
void loop() {
  Blynk.run();
  timer.run();

  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    handlePacket();
  }
}