/**
 * Periodic_Compensated.ino
 * ------------------------
 * Periodic measurement with **compensation**:
 *  - Temperature offset (board self-heating)
 *  - Installation altitude (m)
 *  - Ambient pressure raw (datasheet scaling)
 */

#include <Wire.h>
#include "7Semi_SCD4x.h"

SCD4x_7Semi scd;

const float TEMP_OFFSET_C = 1.50f;
const uint16_t ALTITUDE_M = 30;
/**
 * Optional ambient pressure (raw units per Sensirion spec).
 * Set to >= 0 to apply; keep -1 to skip.
 */
const int PRESS_RAW = -1;

void setup() {
  Serial.begin(115200);
  while (!Serial)
    ;

  Serial.println(F("7Semi SCD4x\n Periodic with compensation"));

  // begin(sda, scl, freq). For default MCUs I2C, keep sda and scl -1.
  while (!scd.begin(-1, -1, 100000)) {
    Serial.println(F("Sensor not detected."));
    delay(1000);
  }

  // ---- Compensation settings ----
  if (!scd.setTemperatureOffset(TEMP_OFFSET_C))
    Serial.println(F("setTemperatureOffset failed"));

  if (!scd.setSensorAltitude(ALTITUDE_M))
    Serial.println(F("setSensorAltitude failed"));

  if (PRESS_RAW >= 0 && !scd.setAmbientPressureRaw((uint16_t)PRESS_RAW))
    Serial.println(F("setAmbientPressureRaw failed"));

  scd.setAutomaticSelfCalibrationEnabled(true);

  if (!scd.startPeriodicMeasurement()) {
    Serial.println(F("startPeriodicMeasurement failed"));
    while (1) delay(1000);
  }
  Serial.println(F("Started. First valid reading in ~5s."));
}
/**
 * - Poll data-ready every ~2 s
 * - When ready, read CO₂ (ppm), Temperature (°C), RH (%)
 * - Print compact line
 */
void loop() {
  static uint32_t t = 0;
  if (millis() - t < 2000) return;  // poll every ~2 s
  t = millis();

  uint16_t st = 0;
  // Many Sensirion refs use (status & 0x07FF) as "new data available"
  if (scd.getDataReadyStatus(st) && (st & 0x07FF)) {
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
    }
  }
}


