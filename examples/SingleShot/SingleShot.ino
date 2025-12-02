/**
 * SingleShot.ino
 * -----------------------
 * Run **single-shot** conversions on a fixed interval (no periodic mode).
 *
 * Notes:
 * - Single-shot is useful for duty-cycling to save power.
 * - Keep a small delay between command and read; we poll data-ready.
 * - If you previously ran periodic mode, be sure to stop it first.
 */

#include <7Semi_SCD4x.h>

SCD4x_7Semi scd;
const uint32_t INTERVAL_MS = 5000;  // single-shot every 5 s

/**
 * - Wait until data-ready bit asserts or timeout elapses
 * - Returns true when ready, false on timeout
 */
static bool waitReady(uint32_t timeout_ms) {
  uint32_t t0 = millis();
  while (millis() - t0 < timeout_ms) {
    uint16_t st = 0;
    if (scd.getDataReadyStatus(st) && (st & 0x07FF)) return true;
    delay(10);
  }
  return false;
}

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println(F("7Semi SCD4x\nSingle-shot at fixed interval"));

  while (!scd.begin(-1, -1, 100000)) {
    Serial.println(F("Sensor not detected."));
    delay(1000);
  }

  // Ensure not in periodic mode before single-shot operation
  scd.stopPeriodicMeasurement();
  delay(500);
}

// ======================= Loop ======================

/**
 * - Every INTERVAL_MS:
 *   * trigger single-shot
 *   * wait for data-ready (with timeout)
 *   * read CO₂ (ppm), Temperature (°C), RH (%)
 */
void loop() {
  static uint32_t t = 0;
  if (millis() - t < INTERVAL_MS) return;
  t = millis();

  if (!scd.measureSingleShot()) {
    Serial.println(F("measureSingleShot failed"));
    return;
  }

  if (!waitReady(3000)) {
    Serial.println(F("timeout waiting for single-shot"));
    return;
  }

  uint16_t co2;
  float tc, rh;
  if (scd.readMeasurement(co2, tc, rh)) {
    Serial.print(F("CO2 "));
    Serial.print(co2);
    Serial.print(F(" ppm  "));
    Serial.print(F("T "));
    Serial.print(tc, 2);
    Serial.print(F(" C  "));
    Serial.print(F("RH "));
    Serial.print(rh, 1);
    Serial.println(F(" %"));
  } else {
    Serial.println(F("Read failed (CRC/I2C)"));
  }
}

/* ===================== Notes =====================
 - For very low power, consider increasing INTERVAL_MS.
 - If you need only RHT without CO₂, use measureSingleShotRhtOnly().
=================================================== */
