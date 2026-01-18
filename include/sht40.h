#pragma once

#include "driver/i2c.h"
#include "esp_log.h"
#include "esp_err.h"

/**
 * @brief Initializes the SHT40 sensor configuration.
 * * This function configures the I2C parameters (SDA, SCL pins, frequency)
 * and installs the I2C driver for the ESP32.
 * It must be called before any read operations.
 */
void sht40_init();

/**
 * @brief Reads temperature and humidity data from the SHT40 sensor.
 * * Sends a "Measure High Precision" command to the sensor, waits for the measurement,
 * reads the data, validates CRC checksums, and updates the internal global variables.
 * * @return uint8_t Returns 1 if the read and CRC check were successful, 0 otherwise.
 */
uint8_t sht40_read();

/**
 * @brief FreeRTOS task handler for sending sensor data via UART.
 * * This task periodically triggers a sensor reading and sends the 
 * temperature and humidity values to the connected UART device.
 * Intended to be passed to xTaskCreate().
 * * @param pvParameter Pointer to task parameters (unused).
 */
void sht40_uart_task(void *pvParameter);

/**
 * @brief FreeRTOS task handler for uploading sensor data to Firebase.
 * * This task periodically triggers a sensor reading and updates
 * the specific paths in the Firebase Realtime Database.
 * Intended to be passed to xTaskCreate().
 * * @param pvParameters Pointer to task parameters (unused).
 */
void sht40_firebase_task(void* pvParameters);