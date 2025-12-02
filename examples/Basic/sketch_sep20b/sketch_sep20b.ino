/**
 * Basic.ino
 * ------------------
 *
 * - periodic/low-power, single-shot,
 *   ASC enable/target/periods, temperature offset, altitude, FRC, NVM persist,
 *   self-test, reinit, power-down/wake, serial number & variant.
 * - Custom I²C pin remap on ESP32 optional alternate I²C bus.
 */

#include <7Semi_SCD4x.h>

// I2C speed for bring-up (100 kHz recommended; raise later if stable)
#define I2C_FREQ_HZ 100000UL

// Set to -1 to use board defaults
#define I2C_SDA_PIN -1
#define I2C_SCL_PIN -1

// Polling period while in periodic modes
#define POLL_MS 2000UL

SCD4x_7Semi scd(&Wire);

uint32_t lastPoll = 0;
bool running = false;
bool lowPower = false;

void setup() {
  Serial.begin(115200);
  while (!Serial) {}

  Serial.println(F("\n== 7Semi SCD4x =="));

  while (!scd.begin(I2C_SDA_PIN, I2C_SCL_PIN, I2C_FREQ_HZ)) {
    Serial.println(F("ERROR: SCD4x not detected @0x62. Check wiring/power."));
    delay(1000);
  }

  // Start in standard periodic mode by default
  if (scd.startPeriodicMeasurement()) {
    running = true;
    lowPower = false;
    Serial.println(F("Started standard periodic mode. First valid reading ~5s."));
  } else {
    Serial.println(F("failed to start periodic mode."));
  }
}

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
