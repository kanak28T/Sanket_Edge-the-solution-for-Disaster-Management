/*
 * NODE-2 — DROUGHT MONITOR
 * Sends at: 10s, 30s, 50s...
 * Node-1 sends at: 0s, 20s, 40s...
 * 10 second offset prevents packet collision
 *
 * REPLACE this section with your actual
 * sensor reading code for temp/humidity/soil
 */

#include <LoRa.h>
#include <DHT.h>              // if using DHT sensor

// ===== LoRa Pins =====
#define LORA_SS    5
#define LORA_RST   14
#define LORA_DIO0  27

// ===== Your Sensor Pins (adjust as needed) =====
#define DHT_PIN     33
#define DHT_TYPE    DHT11
#define SOIL_PIN   32         // analog soil moisture pin

// ===== Timing =====
#define SEND_INTERVAL   20000  // send every 20 seconds
#define MY_TIME_SLOT    10000  // Node-2 waits 10s before first send

DHT dht(DHT_PIN, DHT_TYPE);

int   sendCount  = 0;
int   dryDays    = 0;
float lastSoil   = 100;

// ============================================================
// Read Soil Moisture (0-100%)
// ============================================================
float readSoilMoisture() {
  int raw = analogRead(SOIL_PIN);
  // Calibrate these values for your sensor:
  // 4095 = completely dry, 1500 = completely wet
  float pct = map(raw, 4095, 1500, 0, 100);
  if (pct < 0)   pct = 0;
  if (pct > 100) pct = 100;
  return pct;
}

// ============================================================
// Get Drought Risk based on readings
// ============================================================
String getDroughtRisk(float temp, float humidity, float soil, int dryDays) {
  int score = 0;
  if (soil < 25)      score += 35;
  if (temp > 35)      score += 25;
  if (humidity < 30)  score += 25;
  if (dryDays > 5)    score += 15;

  if (score >= 60) return "HIGH";
  if (score >= 30) return "MEDIUM";
  return "LOW";
}

// ============================================================
// Setup
// ============================================================
void setup() {
  Serial.begin(115200);
  delay(2000);

  Serial.println("\n╔══════════════════════════════════╗");
  Serial.println("║  NODE-2 — DROUGHT MONITOR        ║");
  Serial.println("║  Time slot: sends at 10s, 30s... ║");
  Serial.println("╚══════════════════════════════════╝");

  dht.begin();

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

  // *** KEY FIX: Wait 10 seconds before first send ***
  // This gives Node-1 time to send first, then we alternate
  Serial.println("⏳ Waiting 10s for time slot offset...");
  delay(MY_TIME_SLOT);

  Serial.println("✓ Node-2 ready — sending every 20s\n");
}

// ============================================================
// Loop
// ============================================================
void loop() {
  sendCount++;

  // Read sensors
  float temp     = dht.readTemperature();
  float humidity = dht.readHumidity();
  float soil     = readSoilMoisture();

  // Handle DHT read failure
  if (isnan(temp))     temp     = 0;
  if (isnan(humidity)) humidity = 0;

  // Track dry days
  if (soil < lastSoil - 5) dryDays++;
  else                      dryDays = 0;
  lastSoil = soil;

  // Get drought risk
  float  confidence  = 0.85;
  String droughtRisk = getDroughtRisk(temp, humidity, soil, dryDays);

  Serial.println("\n#" + String(sendCount) +
                 " ══════════════════════════════");
  Serial.print("Temperature : "); Serial.print(temp, 1);     Serial.println(" °C");
  Serial.print("Humidity    : "); Serial.print(humidity, 1);  Serial.println(" %");
  Serial.print("Soil        : "); Serial.print(soil, 1);      Serial.println(" %");
  Serial.print("Dry Days    : "); Serial.println(dryDays);
  Serial.print("Drought Risk: "); Serial.println(droughtRisk);

  // Build JSON — same format receiver expects
  String payload = "{";
  payload += "\"node\":\"NODE-2\",";
  payload += "\"T2M\":"           + String(temp, 2)      + ",";
  payload += "\"humidity\":"      + String(humidity, 2)   + ",";
  payload += "\"soil_moisture\":" + String((int)soil)     + ",";
  payload += "\"dry_days\":"      + String(dryDays)       + ",";
  payload += "\"drought_risk\":\"" + droughtRisk          + "\",";
  payload += "\"confidence\":"    + String(confidence, 2);
  payload += "}";

  // Send via LoRa
  LoRa.beginPacket();
  LoRa.print(payload);
  LoRa.endPacket();

  Serial.println("[LoRa TX] " + payload);
  Serial.println("✓ Sent! Next in 20s...");

  delay(SEND_INTERVAL);
}