/**
 * Periodic_Measurement.ino
 * ---------------------
 * Continuous readout in **standard periodic** mode.
 *
 * - I²C addr: 0x62 (fixed)
 * - First valid sample: ~5 s after start
 * - Poll rate here: 2 s (matches typical update cadence)
 * - ASC enabled (optional; requires regular exposure to fresh air)
 */

#include <7Semi_SCD4x.h>

// ===================== Globals =====================

SCD4x_7Semi scd;   // default I2C bus (&Wire), device address 0x62

/**
 * - Initialize sensor @ 100 kHz (recommended for bring-up)
 * - (Optional) Enable ASC
 * - Start standard periodic measurement
 */
void setup() {
  Serial.begin(115200);
  while (!Serial);

  Serial.println(F("7Semi SCD4x\nPeriodic Standard"));

  // begin(sda, scl, freq). For fixed-pin MCUs, sda/scl are ignored.
  if (!scd.begin(-1, -1, 100000)) {
    Serial.println(F("Sensor not detected..."));
    while (1) delay(1000);
  }
  scd.setAutomaticSelfCalibrationEnabled(true);

  if (!scd.startPeriodicMeasurement()) {
    Serial.println(F("ERROR: startPeriodicMeasurement failed"));
    while (1) delay(1000);
  }
  // if (!scd.startLowPowerPeriodicMeasurement()) {
  //   Serial.println(F("ERROR: startLowPowerPeriodicMeasurement failed"));
  //   while (1) delay(1000);
  // }
  Serial.println(F("Started. First valid reading in ~5s."));
}

/**
 * - Poll data-ready every ~2 s
 * - If ready, read CO₂ (ppm), Temperature (°C), RH (%)
 * - Print in a compact, stable format
 */
void loop() {
  static uint32_t t = 0;
  if (millis() - t < 2000) return;  // poll every ~2 s
  t = millis();

  uint16_t st = 0;
  // Many Sensirion refs use (status & 0x07FF) as "new data available"
  if (scd.getDataReadyStatus(st) && (st & 0x07FF)) {
    uint16_t co2; float tc, rh;
    if (scd.readMeasurement(co2, tc, rh)) {
      Serial.print(F("CO2 ")); Serial.print(co2); Serial.print(F(" ppm  "));
      Serial.print(F("T "));   Serial.print(tc, 2); Serial.print(F(" C  "));
      Serial.print(F("RH "));  Serial.print(rh, 1); Serial.println(F(" %"));
    }
  }
}

/* ===================== Notes =====================
 - If you never get data-ready:
   * Recheck wiring, pull-ups, and I²C pins for your board.
   * Keep I²C at 100 kHz during bring-up.
   * Ensure stable power and short wires.

 - If accuracy drifts:
   * Disable ASC if the sensor never sees fresh air.
   * Consider forced recalibration in a known reference environment.
=================================================== */
