/**
 * 7Semi_SCD4x.cpp
 * ---------------
 * Minimal 7Semi driver for Sensirion SCD4x (SCD40/SCD41) CO₂ sensor.
 *
 * - I²C address: 0x62 (fixed)
 * - Commands: 16-bit big-endian
 * - Data: 16-bit word + CRC-8 (poly 0x31, init 0xFF) per word
 */

#include "7Semi_SCD4x.h"

// ================= Constructor =================

/**
- Construct driver on a given TwoWire bus
- wire   : &Wire / &Wire1 (defaults to &Wire if nullptr)
*/
SCD4x_7Semi::SCD4x_7Semi(TwoWire *wire)
  : i2c(wire ? wire : &Wire){}

// ================= Begin / Init =================

/**
- Initialize I²C and probe sensor
- sda,scl : ESP32/ESP8266 pin remap (pass -1 for board defaults). Other cores ignore.
- i2cFreq : I²C clock in Hz (100 kHz recommended for bring-up)
- return  : true if serial number read OK (device present)
*/
bool SCD4x_7Semi::begin(int sda, int scl, uint32_t i2cFreq) {
#if defined(ARDUINO_ARCH_ESP32)
  i2c->begin((sda >= 0) ? sda : SDA, (scl >= 0) ? scl : SCL, i2cFreq);
#elif defined(ARDUINO_ARCH_ESP8266)
  i2c->begin((sda >= 0) ? sda : SDA, (scl >= 0) ? scl : SCL);
  i2c->setClock(i2cFreq);
#else
  (void)sda;
  (void)scl;
  i2c->begin();
#ifdef TWBR
  i2c->setClock(i2cFreq);
#endif
#endif
  // Bring device to known state
  (void)wakeUp();
  delay(2);
  (void)reInit();
  delay(20);

  // Presence check via serial number
  uint64_t sn;
  if (readSerialNumber(sn)) return true;
  return false;
}

// ================ Measurement Control ================

/**
- Start standard periodic measurement
- note   : ~5 s to first valid sample
*/
bool SCD4x_7Semi::startPeriodicMeasurement() {
  return sendCommand(START_PERIODIC_MEASUREMENT_CMD_ID);
}

/**
- Start low-power periodic measurement
*/
bool SCD4x_7Semi::startLowPowerPeriodicMeasurement() {
  return sendCommand(START_LOW_POWER_PERIODIC_MEASUREMENT_CMD_ID);
}

/**
- Stop periodic measurement
*/
bool SCD4x_7Semi::stopPeriodicMeasurement() {
  return sendCommand(STOP_PERIODIC_MEASUREMENT_CMD_ID);
}

/**
- Get data-ready status
- status_raw : out status word (implementation-specific bits)
- return     : true on success
*/
bool SCD4x_7Semi::getDataReadyStatus(uint16_t &status_raw) {
  return readNData(GET_DATA_READY_STATUS_RAW_CMD_ID, &status_raw, 1);
}

/**
- Read latest CO₂ / temperature / humidity
- co2_ppm    : out CO₂ in ppm (raw word)
- temp_c     : out temperature in °C  (T = -45 + 175 * raw / 65535)
- rh_percent : out relative humidity % (RH = 100 * raw / 65535)
- return     : true on success (CRC + length OK)
*/
bool SCD4x_7Semi::readMeasurement(uint16_t &co2_ppm, float &temp_c, float &rh_percent) {
  if (!sendCommand(READ_MEASUREMENT_RAW_CMD_ID)) return false;
  delay(1);

  // 3 words + 3 CRC bytes = 9 bytes
  uint8_t raw[9];
  i2c->requestFrom((uint8_t)address, (uint8_t)9);
  if (i2c->available() != 9) return false;
  for (size_t i = 0; i < 9; ++i)
    raw[i] = i2c->read();

  uint16_t co2_raw, t_raw, rh_raw;
  if (!checkCrc(raw + 0, co2_raw)) return false;
  if (!checkCrc(raw + 3, t_raw))   return false;
  if (!checkCrc(raw + 6, rh_raw))  return false;

  co2_ppm   = co2_raw;
  temp_c    = -45.0f + 175.0f * (float)t_raw / 65535.0f;
  rh_percent= 100.0f * (float)rh_raw / 65535.0f;
  return true;
}

/**
- Trigger single-shot CO₂ + RHT measurement (no read here)
*/
bool SCD4x_7Semi::measureSingleShot() {
  return sendCommand(MEASURE_SINGLE_SHOT_CMD_ID);
}

/**
- Trigger single-shot RHT-only measurement
*/
bool SCD4x_7Semi::measureSingleShotRhtOnly() {
  return sendCommand(MEASURE_SINGLE_SHOT_RHT_ONLY_CMD_ID);
}

// ================= Configuration =================

/**
- Set temperature offset (°C)
- degC  : offset to apply
*/
bool SCD4x_7Semi::setTemperatureOffset(float degC) {
  uint16_t raw = (uint16_t)((degC / 175.0f) * 65535.0f);
  return writeCommand(SET_TEMPERATURE_OFFSET_RAW_CMD_ID, &raw, 1);
}

/**
- Get temperature offset (°C)
- degC : out offset
*/
bool SCD4x_7Semi::getTemperatureOffset(float &degC) {
  uint16_t raw;
  if (!readNData(GET_TEMPERATURE_OFFSET_RAW_CMD_ID, &raw, 1)) return false;
  degC = 175.0f * (float)raw / 65535.0f;
  return true;
}

/**
- Set installation altitude (meters)
*/
bool SCD4x_7Semi::setSensorAltitude(uint16_t meters) {
  return writeCommand(SET_SENSOR_ALTITUDE_CMD_ID, &meters, 1);
}

/**
- Get installation altitude (meters)
*/
bool SCD4x_7Semi::getSensorAltitude(uint16_t &meters) {
  return readNData(GET_SENSOR_ALTITUDE_CMD_ID, &meters, 1);
}

/**
- Set ambient pressure raw (per datasheet)
- mbar_raw : raw scaling (external compensation only)
*/
bool SCD4x_7Semi::setAmbientPressureRaw(uint16_t mbar_raw) {
  return writeCommand(SET_AMBIENT_PRESSURE_RAW_CMD_ID, &mbar_raw, 1);
}

/**
- Get ambient pressure raw
*/
bool SCD4x_7Semi::getAmbientPressureRaw(uint16_t &mbar_raw) {
  return readNData(GET_AMBIENT_PRESSURE_RAW_CMD_ID, &mbar_raw, 1);
}

// ================= ASC (Auto Self-Calibration) =================

/**
- Enable/disable ASC
*/
bool SCD4x_7Semi::setAutomaticSelfCalibrationEnabled(bool enable) {
  uint16_t on = enable ? 1 : 0;
  return writeCommand(SET_AUTOMATIC_SELF_CALIBRATION_ENABLED_CMD_ID, &on, 1);
}

/**
- Read ASC enable flag
- enabled : out true/false
*/
bool SCD4x_7Semi::getAutomaticSelfCalibrationEnabled(bool &enabled) {
  uint16_t val;
  if (!readNData(GET_AUTOMATIC_SELF_CALIBRATION_ENABLED_CMD_ID, &val, 1)) return false;
  enabled = (val != 0);
  return true;
}

/**
- Set ASC target CO₂ (ppm)
*/
bool SCD4x_7Semi::setAutomaticSelfCalibrationTarget(uint16_t ppm) {
  return writeCommand(SET_AUTOMATIC_SELF_CALIBRATION_TARGET_CMD_ID, &ppm, 1);
}

/**
- Get ASC target CO₂ (ppm)
*/
bool SCD4x_7Semi::getAutomaticSelfCalibrationTarget(uint16_t &ppm) {
  return readNData(GET_AUTOMATIC_SELF_CALIBRATION_TARGET_CMD_ID, &ppm, 1);
}

/**
- Set ASC initial period (hours)
*/
bool SCD4x_7Semi::setAutomaticSelfCalibrationInitialPeriod(uint16_t hours) {
  return writeCommand(SET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD_CMD_ID, &hours, 1);
}

/**
- Get ASC initial period (hours)
*/
bool SCD4x_7Semi::getAutomaticSelfCalibrationInitialPeriod(uint16_t &hours) {
  return readNData(GET_AUTOMATIC_SELF_CALIBRATION_INITIAL_PERIOD_CMD_ID, &hours, 1);
}

/**
- Set ASC standard period (hours)
*/
bool SCD4x_7Semi::setAutomaticSelfCalibrationStandardPeriod(uint16_t hours) {
  return writeCommand(SET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD_CMD_ID, &hours, 1);
}

/**
- Get ASC standard period (hours)
*/
bool SCD4x_7Semi::getAutomaticSelfCalibrationStandardPeriod(uint16_t &hours) {
  return readNData(GET_AUTOMATIC_SELF_CALIBRATION_STANDARD_PERIOD_CMD_ID, &hours, 1);
}

/**
- Forced recalibration to a known reference
- reference_ppm : known ambient CO₂ (stable environment)
- frc_result    : optional device return value
*/
bool SCD4x_7Semi::performForcedRecalibration(uint16_t reference_ppm, uint16_t *frc_result) {
  if (!writeCommand(PERFORM_FORCED_RECALIBRATION_CMD_ID, &reference_ppm, 1)) return false;
  if (frc_result) {
    delay(1);
    if (!readNData(PERFORM_FORCED_RECALIBRATION_CMD_ID, frc_result, 1)) return false;
  }
  return true;
}

// ================= Maintenance / Identity =================

/**
- Persist current settings to NVM
*/
bool SCD4x_7Semi::persistSettings() {
  return sendCommand(PERSIST_SETTINGS_CMD_ID);
}

/**
- Read 48-bit serial number (3 words + CRC)
- sn : out 48-bit serial (w0|w1|w2)
*/
bool SCD4x_7Semi::readSerialNumber(uint64_t &sn) {
  if (!sendCommand(GET_SERIAL_NUMBER_CMD_ID)) return false;
  delay(1);

  uint8_t raw[9];
  i2c->requestFrom((uint8_t)address, (uint8_t)9);
  if (i2c->available() != 9) return false;
  for (size_t i = 0; i < 9; ++i)
    raw[i] = i2c->read();

  uint16_t w0, w1, w2;
  if (!checkCrc(raw + 0, w0)) return false;
  if (!checkCrc(raw + 3, w1)) return false;
  if (!checkCrc(raw + 6, w2)) return false;

  sn = ((uint64_t)w0 << 32) | ((uint64_t)w1 << 16) | (uint64_t)w2;
  return true;
}

/**
- Read sensor variant raw word
*/
bool SCD4x_7Semi::getSensorVariantRaw(uint16_t &variant) {
  return readNData(GET_SENSOR_VARIANT_RAW_CMD_ID, &variant, 1);
}

/**
- Run self-test (blocking); returns a status word
*/
bool SCD4x_7Semi::performSelfTest(uint16_t &status_word) {
  return readNData(PERFORM_SELF_TEST_CMD_ID, &status_word, 1);
}

/**
- Factory reset
*/
bool SCD4x_7Semi::factoryReset() {
  return sendCommand(PERFORM_FACTORY_RESET_CMD_ID);
}

/**
- Re-initialize device
*/
bool SCD4x_7Semi::reInit() {
  return sendCommand(REINIT_CMD_ID);
}

// ================= Power =================

/**
- Enter low-power mode
*/
bool SCD4x_7Semi::powerDown() {
  return sendCommand(POWER_DOWN_CMD_ID);
}

/**
- Wake sensor from low-power mode
*/
bool SCD4x_7Semi::wakeUp() {
  return sendCommand(WAKE_UP_CMD_ID);
}

// ================= Low-level helpers =================

/**
- Sensirion CRC-8 for one data word
- poly  : 0x31
- init  : 0xFF
- return: crc byte
*/
uint8_t SCD4x_7Semi::crc8(uint8_t b0, uint8_t b1) {
  uint8_t crc = 0xFF;
  crc ^= b0;
  for (int i = 0; i < 8; ++i) crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
  crc ^= b1;
  for (int i = 0; i < 8; ++i) crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
  return crc;
}

/**
- Convenience wrapper: command without payload
*/
bool SCD4x_7Semi::sendCommand(uint16_t cmd) {
  return txCommand(cmd, nullptr, 0);
}

/**
- Send command with N data words (each followed by CRC)
*/
bool SCD4x_7Semi::writeCommand(uint16_t cmd, const uint16_t *words, size_t nwords) {
  return txCommand(cmd, words, nwords);
}

/**
- Read N data words (with CRC) after issuing a command
- cmd    : command code
- out    : buffer for nwords (16-bit each)
- nwords : number of words to read
- return : true when length and CRCs match
*/
bool SCD4x_7Semi::readNData(uint16_t cmd, uint16_t *out, size_t nwords) {
  if (!sendCommand(cmd)) return false;
  delay(1);

  const size_t nbytes = nwords * 3; // 2 data + 1 CRC per word
  i2c->requestFrom((uint8_t)address, (uint8_t)nbytes);
  if (i2c->available() != (int)nbytes) return false;

  for (size_t i = 0; i < nwords; ++i) {
    uint8_t b0 = i2c->read();
    uint8_t b1 = i2c->read();
    uint8_t crc = i2c->read();
    if (crc8(b0, b1) != crc) return false;
    out[i] = (uint16_t(b0) << 8) | uint16_t(b1);
  }
  return true;
}

/**
- Transmit a command and optional payload
- cmd    : 16-bit big-endian command
- words  : optional payload words (nullptr if none)
- nwords : number of payload words
- return : true if endTransmission() == 0
*/
bool SCD4x_7Semi::txCommand(uint16_t cmd, const uint16_t *words, size_t nwords) {
  i2c->beginTransmission(address);
  i2c->write(uint8_t(cmd >> 8));   // MSB
  i2c->write(uint8_t(cmd & 0xFF)); // LSB
  for (size_t i = 0; i < nwords; ++i) {
    uint8_t b0 = uint8_t(words[i] >> 8);
    uint8_t b1 = uint8_t(words[i] & 0xFF);
    uint8_t c  = crc8(b0, b1);
    i2c->write(b0);
    i2c->write(b1);
    i2c->write(c);
  }
  return (i2c->endTransmission() == 0);
}

/**
- Parse a word + CRC from a 3-byte slice
- p    : pointer to [MSB, LSB, CRC]
- word : out 16-bit value (big-endian)
- return : true if CRC matches
*/
bool SCD4x_7Semi::checkCrc(const uint8_t *p, uint16_t &word) {
  uint8_t b0 = p[0], b1 = p[1], c = p[2];
  if (crc8(b0, b1) != c) return false;
  word = (uint16_t(b0) << 8) | uint16_t(b1);
  return true;
}
