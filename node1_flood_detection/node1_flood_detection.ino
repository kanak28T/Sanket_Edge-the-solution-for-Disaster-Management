#include <FloodMonitor-ESP32_inferencing.h>
#include <LoRa.h>

// ===== JSN-SR04T Pins =====
#define TRIG_PIN  12
#define ECHO_PIN  21

// ===== LoRa Pins =====
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  27

// ===== Tank Config =====
#define MAX_DISTANCE_CM   28.0
#define SENSOR_TO_BOTTOM  28.0

// ===== Timing =====
#define SEND_INTERVAL     30000   // 30 seconds
// Node-1 has NO offset — sends immediately

// ===== Rainfall Thresholds (% rise per 30s) =====
#define RAIN_LIGHT     2.0    // 2% rise = light rain
#define RAIN_MODERATE  5.0    // 5% rise = moderate rain
#define RAIN_HEAVY    10.0    // 10%+ rise = heavy rain 🚨

// ===== History for Rainfall Detection =====
#define HISTORY_SIZE   5      // track last 5 readings
float levelHistory[HISTORY_SIZE] = {0};
int   historyIndex = 0;
bool  historyFull  = false;

int sendCount = 0;

// ============================================================
// Read Distance (average 5 samples)
// ============================================================
float readDistanceCM() {
  float total = 0;
  int   valid = 0;

  for (int i = 0; i < 5; i++) {
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);

    long dur = pulseIn(ECHO_PIN, HIGH, 30000);
    if (dur > 0) {
      total += (dur * 0.034) / 2.0;
      valid++;
    }
    delay(50);
  }

  if (valid == 0) return -1;
  return total / valid;
}

// ============================================================
// Distance → Water Level %
// ============================================================
float getWaterLevelPct(float distanceCM) {
  if (distanceCM <= 0) return 0;
  float waterHeight = SENSOR_TO_BOTTOM - distanceCM;
  float pct = (waterHeight / MAX_DISTANCE_CM) * 100.0;
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

// ============================================================
// Update Level History
// ============================================================
void updateHistory(float level) {
  levelHistory[historyIndex] = level;
  historyIndex = (historyIndex + 1) % HISTORY_SIZE;
  if (historyIndex == 0) historyFull = true;
}

// ============================================================
// Calculate Rise Rate (% per interval)
// Compares current level to oldest stored level
// ============================================================
float calculateRiseRate(float currentLevel) {
  // Need at least 2 readings
  if (!historyFull && historyIndex < 2) return 0;

  // Get oldest reading in history
  float oldestLevel;
  if (historyFull) {
    oldestLevel = levelHistory[historyIndex]; // next slot = oldest
  } else {
    oldestLevel = levelHistory[0];
  }

  return currentLevel - oldestLevel;
}

// ============================================================
// Detect Rainfall Intensity
// ============================================================
String getRainfallStatus(float riseRate) {
  if (riseRate >= RAIN_HEAVY)    return "HEAVY_RAIN";
  if (riseRate >= RAIN_MODERATE) return "MODERATE_RAIN";
  if (riseRate >= RAIN_LIGHT)    return "LIGHT_RAIN";
  if (riseRate < -2.0)           return "DRAINING";
  return "NO_RAIN";
}

// ============================================================
// Get Rainfall Confidence (0.0 - 1.0)
// ============================================================
float getRainfallConfidence(float riseRate) {
  if (riseRate >= RAIN_HEAVY)    return 0.95;
  if (riseRate >= RAIN_MODERATE) return 0.85;
  if (riseRate >= RAIN_LIGHT)    return 0.75;
  return 0.90;  // confident it's not raining
}

// ============================================================
// Print Rainfall Alert
// ============================================================
void printRainfallAlert(String rainfall, float riseRate) {
  Serial.println("\n┌─────────────────────────────────┐");
  Serial.print  ("│ RAINFALL: ");
  Serial.println(rainfall);
  Serial.print  ("│ Rise Rate: +");
  Serial.print  (riseRate, 1);
  Serial.println("% per 30s");

  if (rainfall == "HEAVY_RAIN") {
    Serial.println("│ 🚨 HEAVY RAINFALL DETECTED!     │");
    Serial.println("│    Flood risk is HIGH!          │");
  } else if (rainfall == "MODERATE_RAIN") {
    Serial.println("│ ⚠️  Moderate rainfall detected   │");
    Serial.println("│    Monitor closely              │");
  } else if (rainfall == "LIGHT_RAIN") {
    Serial.println("│ 🌧️  Light rain detected          │");
  } else if (rainfall == "DRAINING") {
    Serial.println("│ ↓  Water level falling          │");
  } else {
    Serial.println("│ ✓  No rainfall detected         │");
  }
  Serial.println("└─────────────────────────────────┘");
}

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n╔════════════════════════════════════════╗");
  Serial.println("║  NODE-1 — FLOOD + RAINFALL MONITOR    ║");
  Serial.println("║  Sends: 0s, 30s, 60s...               ║");
  Serial.println("╚════════════════════════════════════════╝");

  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  LoRa.setPins(LORA_SS, LORA_RST, LORA_DIO0);
  if (!LoRa.begin(865E6)) {
    Serial.println("❌ LoRa init failed");
    while (1) { delay(100); }
  }

  LoRa.setSignalBandwidth(125E3);
  LoRa.setCodingRate4(5);
  LoRa.setSpreadingFactor(7);
  LoRa.enableCrc();
  LoRa.setTxPower(20);

  Serial.println("✓ LoRa ready at 865 MHz");
  Serial.println("✓ Rainfall detection ready");
  Serial.println("\nThresholds:");
  Serial.println("  LIGHT RAIN    : +2%  per 30s");
  Serial.println("  MODERATE RAIN : +5%  per 30s");
  Serial.println("  HEAVY RAIN    : +10% per 30s 🚨");
  Serial.println("\nStarting...\n");
}

// ============================================================
// Loop
// ============================================================
void loop() {
  sendCount++;

  // Read sensor
  float distance      = readDistanceCM();
  float waterLevelPct = getWaterLevelPct(distance);

  // Calculate rise rate BEFORE updating history
  float  riseRate = calculateRiseRate(waterLevelPct);
  String rainfall = getRainfallStatus(riseRate);
  float  rainConf = getRainfallConfidence(riseRate);

  // Now update history with current reading
  updateHistory(waterLevelPct);

  Serial.println("\n#" + String(sendCount) +
                 " ══════════════════════════════════");
  Serial.print("Distance    : "); Serial.print(distance, 1);
  Serial.println(" cm");
  Serial.print("Water Level : "); Serial.print(waterLevelPct, 1);
  Serial.println(" %");
  Serial.print("Rise Rate   : ");
  if (riseRate >= 0) Serial.print("+");
  Serial.print(riseRate, 1); Serial.println("% since last");

  // Run Edge Impulse classifier
  float features[] = { waterLevelPct };
  signal_t signal;
  int err = numpy::signal_from_buffer(features, 1, &signal);
  if (err != 0) {
    Serial.println("❌ Signal error");
    delay(SEND_INTERVAL);
    return;
  }

  ei_impulse_result_t result;
  EI_IMPULSE_ERROR res = run_classifier(&signal, &result, false);
  if (res != EI_IMPULSE_OK) {
    Serial.println("❌ Classifier failed");
    delay(SEND_INTERVAL);
    return;
  }

  // Find best label
  float maxVal     = 0;
  const char* tier = "UNKNOWN";
  for (int i = 0; i < EI_CLASSIFIER_LABEL_COUNT; i++) {
    if (result.classification[i].value > maxVal) {
      maxVal = result.classification[i].value;
      tier   = result.classification[i].label;
    }
  }

  Serial.print("Flood Tier  : "); Serial.println(tier);
  Serial.print("Confidence  : "); Serial.println(maxVal, 2);

  // Print rainfall alert
  printRainfallAlert(rainfall, riseRate);

  // Build JSON — includes rainfall data
  String payload = "{";
  payload += "\"node\":\"NODE-1\",";
  payload += "\"water_level_pct\":"  + String(waterLevelPct, 2) + ",";
  payload += "\"tier\":\""           + String(tier)             + "\",";
  payload += "\"confidence\":"       + String(maxVal, 2)        + ",";
  payload += "\"rainfall\":\""       + rainfall                 + "\",";
  payload += "\"rise_rate\":"        + String(riseRate, 2)      + ",";
  payload += "\"rain_confidence\":"  + String(rainConf, 2);
  payload += "}";

  // Send via LoRa
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();

  Serial.println("\n[LoRa TX] " + payload);
  Serial.println("✓ Sent! Next in 30s...");

  delay(SEND_INTERVAL);
}