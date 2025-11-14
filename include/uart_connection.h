#pragma once

#include "dht11.h"
#include "driver/uart.h"

/**
 * @file uart_connection.h
 * @brief UART module for sending DHT11 sensor data.
 */

/**
 * @brief Initialize UART peripheral for communication with STM32.
 * 
 * Configures UART with parameters:
 * - Baud rate: 115200
 * - Data bits: 8
 * - Parity: None
 * - Stop bits: 1
 * - Flow control: None
 * 
 * Sets TX to GPIO17 and RX to GPIO16 (changeable).
 */
void uart_init();

/**
 * @brief Send DHT11 temperature and humidity data over UART.
 * 
 * @param temperature The temperature value in Celsius.
 * @param humidity The humidity value in percentage.
 * 
 * Sends a string in the format "T:<temperature>;H:<humidity>\n", 
 * e.g. "T:23.45;H:55.67\n".
 */
void send_dht_data(float temperature, float humidity);

/**
 * @brief FreeRTOS task that reads DHT11 sensor and sends data via UART.
 * 
 * This task repeatedly:
 * 1. Reads temperature and humidity from DHT11 using `dht11_read()`.
 * 2. If read is successful, sends data using `send_dht_data()`.
 * 3. Logs a warning if the read fails.
 * 4. Waits 5 min before next read.
 * 
 * @param pvParameters FreeRTOS task parameters (unused, can be NULL).
 */
void dht_uart_task(void *pvParameters);

void uart_pc_receive_task(void *pvParameters);
