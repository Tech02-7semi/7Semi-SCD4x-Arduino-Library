/**
 * Basic.ino
 * -----------------------
 * Quick-start example for 7Semi SCD4x (SCD40/SCD41) CO₂ sensor.
 *
 * - I²C address : 0x62 (fixed)
 * - First valid reading after startPeriodicMeasurement(): ~5 s
 * - Data rate   : new sample about every 5 s (standard mode)
 *
 * Wiring (typical):
 * - UNO/Nano:  SDA=A4,  SCL=A5,  VCC=3V3/5V*, GND=GND
 * - ESP32:    SDA=21,   SCL=22
 * - ESP8266:  SDA=D2,   SCL=D1
 *   *Use a breakout with level shifting if needed; follow your board’s limits.
 *
 * Tips:
 * - Keep I²C at 100 kHz for bring-up; 400 kHz is fine once stable.
 * - Avoid long leads/noisy wiring; ensure pull-ups on SDA/SCL (2.2k–10k).
 * - ASC is enabled below; use only if your environment has regular fresh air.
 */

#include <7Semi_SCD4x.h>

// ===================== Globals =====================

SCD4x_7Semi scd;  // uses default &Wire and address 0x62

// ===================== Setup =======================

/**
 - Bring up Serial and the sensor
 - Start periodic measurement (standard mode)
 - ASC enabled (optional; disable if you need absolute accuracy in closed rooms)
*/
void setup() {
  Serial.begin(115200);
  while (!Serial) {}  // wait for USB CDC (if applicable)

  Serial.println("SCD4x CO₂ Sensor Example");

  // Initialize the sensor on default pins/clock (see library for pin remap)
  if (!scd.begin()) {
    Serial.println("ERROR: SCD4x not detected. Check wiring and power!");
    while (1) delay(1000);
  }

  // Start periodic measurements (or use startLowPowerPeriodicMeasurement)
  if (!scd.startPeriodicMeasurement()) {
    Serial.println("Failed to start periodic measurement");
    while (1) delay(1000);
  }

  // Optional: enable Automatic Self-Calibration (requires periodic exposure to fresh air)
  scd.setAutomaticSelfCalibrationEnabled(true);

  Serial.println("Started measurement. Waiting for first valid reading (~5s).");
}

// ====================== Loop =======================

/**
 - Poll data-ready every 2 s
 - When ready, read CO₂ (ppm), temperature (°C), humidity (%RH)
 - Prints values to Serial (with basic formatting)
*/
void loop() {
  static uint32_t lastCheck = 0;
  if (millis() - lastCheck < 2000) return;  // every 2 s
  lastCheck = millis();

  uint16_t ready = 0;
  // Bit 10..0 used as "new data available" in many Sensirion examples
  if (scd.getDataReadyStatus(ready) && (ready & 0x07FF)) {
    uint16_t co2;
    float temp, rh;

    if (scd.readMeasurement(co2, temp, rh)) {
      Serial.print("CO₂: ");
      Serial.print(co2);
      Serial.print(" ppm\t");

      Serial.print("Temp: ");
      Serial.print(temp, 2);
      Serial.print(" °C\t");

      Serial.print("RH: ");
      Serial.print(rh, 1);
      Serial.println(" %");
    } else {
      Serial.println("Read failed (CRC/I2C error)");
    }
  }
}

/* ===================== Notes =====================
 - If you never see data ready:
   * Re-check wiring, pull-ups, and I²C pins for your board.
   * Try 100 kHz I²C during bring-up.
   * Ensure stable power; SCD4x can run from 3.3–5.5 V depending on module.

 - If values look offset:
   * Consider setTemperatureOffset() to compensate board self-heating.
   * Avoid strong airflow directly on the sensor.

 - If you need single-shot mode:
   * Use measureSingleShot()/measureSingleShotRhtOnly() and then readMeasurement().
=================================================== */
