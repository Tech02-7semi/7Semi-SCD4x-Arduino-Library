/************************************************************
 * @file    ConfigExample.ino
 * @brief   Basic usage example for the 7semi SHT4x sensor
 *
 * @details
 *   Demonstrates:
 *     - Initializing the SHT4x sensor
 *     - Setting measurement precision (repeatability)
 *     - Configuring the on-board heater (optional)
 *     - Reading and printing temperature & humidity values
 *
 * Hardware Setup:
 *   - Connect SHT4x sensor via I2C (SCL, SDA)
 *   - Use correct I2C pins for your microcontroller
 *
 * Notes:
 *   - Precision options:
 *       REPEATABILITY_LOW    → Fastest, lowest power, less accurate
 *       REPEATABILITY_MEDIUM → Balanced (recommended default)
 *       REPEATABILITY_HIGH   → Slowest, highest power, most accurate
 *
 *   - Heater options:
 *       HEATER_20mW_0_1s, HEATER_20mW_1s
 *       HEATER_110mW_0_1s, HEATER_110mW_1s
 *       HEATER_200mW_0_1s, HEATER_200mW_1s
 *
 * @author  
 *   7semi
 *
 * @license 
 *   MIT License
 *   Copyright (c) 2025 7semi
 ************************************************************/

#include <7semi_SHT4x.h>
#include <Wire.h>
#define scl A4
#define sda A5
SHT4x_7semi sensor;

void setup() {
  Serial.begin(115200);
  Wire.begin();

  if (sensor.begin(Wire,scl,sda)) {             // default clock) {
    Serial.println("SHT4x initialized successfully!");
  } else {
    Serial.println("Failed to initialize SHT4x.");
    while (1);
  }

  // Set measurement precision: medium (balanced speed, power, accuracy)
  sensor.setPrecision(REPEATABILITY_MEDIUM);

  // Turn heater ON: 200 mW power for 1 second
  sensor.setHeater(HEATER_200mW_1s);

  delay(2000); // wait for sensor to stabilize
}

void loop() {
  float temp, hum;

  if (sensor.readTemperatureHumidity(temp, hum)) {
    Serial.print("Temperature: ");
    Serial.print(temp, 2);
    Serial.print(" °C, Humidity: ");
    Serial.print(hum, 2);
    Serial.println(" %RH");
  } else {
    Serial.println("Failed to read temperature and humidity.");
  }

  delay(1000); // read every 1 second
}
