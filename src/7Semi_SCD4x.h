#ifndef _7Semi_SCD4X_H
#define _7Semi_SCD4X_H

#include <Arduino.h>
#include <Wire.h>

/**
 * 7Semi_SCD4x.h
 * --------------
 * Minimal 7Semi driver for Sensirion SCD4x (SCD40/SCD41) CO₂ sensor.
 *
 * - I²C address : 0x62 (fixed)
 * - Commands    : 16-bit big-endian
 * - Data words  : 16-bit big-endian + CRC-8 per word (poly 0x31, init 0xFF)
 *
 * Notes:
 * - Keep bus at 100 kHz for bring-up; higher is fine once stable.
 * - Each 16-bit payload word on the wire is followed by its own CRC byte.
 * - Temperature/Humidity conversion is handled in readMeasurement().
 */

// ===================== SCD4x Command IDs =====================
#define START_PERIODIC_MEASUREMENT_CMD_ID 0x21B1
#define READ_MEASUREMENT_RAW_CMD_ID 0xEC05
#define STOP_PERIODIC_MEASUREMENT_CMD_ID 0x3F86
#define SET_TEMPERATURE_OFFSET_RAW_CMD_ID 0x241D
#define GET_TEMPERATURE_OFFSET_RAW_CMD_ID 0x2318
#define SET_SENSOR_ALTITUDE_CMD_ID 0x2427
#define GET_SENSOR_ALTITUDE_CMD_ID 0x2322
#define SET_AMBIENT_PRESSURE_RAW_CMD_ID 0xE000
#define GET_AMBIENT_PRESSURE_RAW_CMD_ID 0xE000
#define PERFORM_FORCED_RECALIBRATION_CMD_ID 0x362F
#define SET_AUTOMATIC_SELF_CALIBRATION_ENABLED_CMD_ID 0x2416
#define GET_AUTOMATIC_SELF_CALIBRATION_ENABLED_CMD_ID 0x2313
#define SET_AUTOMATIC_SELF_CALIBRATION_TARGET_CMD_ID 0x243A
#define GET_AUTOMATIC_SELF_CALIBRATION_TARGET_CMD_ID 0x233F
#define START_LOW_POWER_PERIODIC_MEASUREMENT_CMD_ID 0x21AC
#define GET_DATA_READY_STATUS_RAW_CMD_ID 0xE4B8
#define PERSIST_SETTINGS_CMD_ID 0x3615
#define GET_SERIAL_NUMBER_CMD_ID 0x3682
#define PERFORM_SELF_TEST_CMD_ID 0x3639
#define PERFORM_FACTORY_RESET_CMD_ID 0x3632
#define REINIT_CMD_ID 0x3646
#define GET_SENSOR_VARIANT_RAW_CMD_ID 0x202F
#define MEASURE_SINGLE_SHOT_CMD_ID 0x219D
#define MEASURE_SINGLE_SHOT_RHT_ONLY_CMD_ID 0x2196
#define POWER_DOWN_CMD_ID 0x36E0
#define WAKE_UP_CMD_ID 0x36F6
#define SET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD_CMD_ID 0x2445
#define GET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD_CMD_ID 0x2340
#define SET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD_CMD_ID 0x244E
#define GET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD_CMD_ID 0x234B

// ========================= Class =========================
class SCD4x_7Semi {
public:
  /**
   * - Construct driver bound to a TwoWire bus
   * - wire : &Wire / &Wire1 (if nullptr, defaults to &Wire)
   */
  SCD4x_7Semi(TwoWire *wire = &Wire);

  /**
   * - Initialize I²C and probe the sensor
   * - sda,scl : ESP32/ESP8266 pin remap (pass -1 to use board defaults)
   * - i2cFreq : I²C frequency in Hz (100000 recommended for bring-up)
   * - return  : true if serial-number read succeeds
   */
  bool begin(int sda = -1, int scl = -1, uint32_t i2cFreq = 10000);

  // ----------------- Measurement control -----------------
  /** - Start standard periodic measurement */
  bool startPeriodicMeasurement();
  /** - Start low-power periodic measurement */
  bool startLowPowerPeriodicMeasurement();
  /** - Stop periodic measurement */
  bool stopPeriodicMeasurement();
  /**
   * - Read data-ready status word
   * - status_raw : out raw status (device-specific bitfields)
   */
  bool getDataReadyStatus(uint16_t &status_raw);
  /**
   * - Read latest CO₂ / T / RH sample
   * - co2_ppm    : out CO₂ in ppm (raw word)
   * - temp_c     : out °C (T = -45 + 175 * raw / 65535)
   * - rh_percent : out %RH (RH = 100 * raw / 65535)
   */
  bool readMeasurement(uint16_t &co2_ppm, float &temp_c, float &rh_percent);
  /** - Trigger single-shot CO₂+RHT measurement (no read here) */
  bool measureSingleShot();
  /** - Trigger single-shot RHT-only measurement */
  bool measureSingleShotRhtOnly();

  // -------------------- Configuration --------------------
  /** - Set temperature offset in °C */
  bool setTemperatureOffset(float degC);
  /** - Get temperature offset in °C */
  bool getTemperatureOffset(float &degC);
  /** - Set installation altitude in meters */
  bool setSensorAltitude(uint16_t meters);
  /** - Get installation altitude in meters */
  bool getSensorAltitude(uint16_t &meters);
  /** - Set ambient pressure (raw, per datasheet) */
  bool setAmbientPressureRaw(uint16_t mbar_raw);
  /** - Get ambient pressure (raw) */
  bool getAmbientPressureRaw(uint16_t &mbar_raw);

  // ------------------------- ASC -------------------------
  /** - Enable/disable Automatic Self-Calibration */
  bool setAutomaticSelfCalibrationEnabled(bool enable);
  /** - Read ASC enabled flag */
  bool getAutomaticSelfCalibrationEnabled(bool &enabled);
  /** - Set ASC target CO₂ in ppm */
  bool setAutomaticSelfCalibrationTarget(uint16_t ppm);
  /** - Get ASC target CO₂ in ppm */
  bool getAutomaticSelfCalibrationTarget(uint16_t &ppm);
  /** - Set ASC initial period in hours */
  bool setAutomaticSelfCalibrationInitialPeriod(uint16_t hours);
  /** - Get ASC initial period in hours */
  bool getAutomaticSelfCalibrationInitialPeriod(uint16_t &hours);
  /** - Set ASC standard period in hours */
  bool setAutomaticSelfCalibrationStandardPeriod(uint16_t hours);
  /** - Get ASC standard period in hours */
  bool getAutomaticSelfCalibrationStandardPeriod(uint16_t &hours);
  bool performForcedRecalibration(uint16_t reference_ppm, uint16_t *frc_result);
  // ------------------- Maintenance/ID --------------------
  /** - Persist current settings to NVM */
  bool persistSettings();
  /** - Read 48-bit serial number (3 words with CRC) */
  bool readSerialNumber(uint64_t &sn);
  /** - Read sensor variant raw word */
  bool getSensorVariantRaw(uint16_t &variant);
  /** - Run built-in self-test (blocking) */
  bool performSelfTest(uint16_t &status_word);
  /** - Restore factory defaults */
  bool factoryReset();
  /** - Re-initialize device */
  bool reInit();

  // ------------------------ Power ------------------------
  /** - Enter low-power mode */
  bool powerDown();
  /** - Wake from low-power mode */
  bool wakeUp();

private:
  // I²C handle and fixed device address (0x62)
  TwoWire *i2c;
  uint8_t address = 0x62;

  // --------------- Low-level primitives ---------------
  /** - Sensirion CRC-8 (poly 0x31, init 0xFF) for one 16-bit word */
  static uint8_t crc8(uint8_t b0, uint8_t b1);
  /** - Convenience: send command with no payload */
  bool sendCommand(uint16_t cmd);
  /** - Send command with N payload words (each word followed by CRC) */
  bool writeCommand(uint16_t cmd, const uint16_t *words, size_t nwords);
  /**
   * - Issue command then read N words (each word validated with CRC)
   * - out must point to buffer of size nwords
   */
  bool readNData(uint16_t cmd, uint16_t *out, size_t nwords);
  /**
   * - Transmit 16-bit command + optional words (with per-word CRC)
   * - return true if endTransmission() == 0
   */
  bool txCommand(uint16_t cmd, const uint16_t *words, size_t nwords);
  /** - Read raw bytes (utility) */
  bool readBytes(uint8_t *buf, size_t n);
  /** - Parse [MSB,LSB,CRC] into word with CRC check */
  static bool checkCrc(const uint8_t *p, uint16_t &word);
};

#endif  // _7Semi_SCD4X_H
