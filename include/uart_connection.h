#pragma once

#include "sht40.h"
#include "driver/uart.h"
#include <time.h>

/**
 * @file uart_connection.h
 * @brief UART module for sending DHT11 sensor data.
 */

 /**
 * @brief Initialize the UART peripheral for communication.
 *
 * This function configures and installs the UART driver for the ESP32. It sets
 * up the UART parameters such as baud rate, data bits, parity, stop bits, and
 * flow control. Additionally, it installs the UART driver and configures the
 * TX and RX pins. A FreeRTOS queue is created to handle UART events in an
 * interrupt-driven manner.
 *
 * ## Configuration Details:
 * - UART port: UART_NUM_1
 * - TX pin: 17
 * - RX pin: 16
 * - Baud rate: 115200
 * - Data bits: 8
 * - Parity: None
 * - Stop bits: 1
 * - Flow control: Disabled
 * - Event queue length: 20
 */
void uart_init();

/**
 * @brief Send SHT40 temperature and humidity data over UART.
 * 
 * @param temperature The temperature value in Celsius.
 * @param humidity The humidity value in percentage.
 * 
 * Sends a string in the format "T:<temperature>;H:<humidity>\n", 
 * e.g. "T:23.45;H:55.67\n".
 */
void uart_sendSensorsData(float temperature, float humidity);

/**
 * @brief UART event handling task.
 *
 * This task waits for UART events sent from the UART ISR (Interrupt Service Routine)
 * via a FreeRTOS queue. It handles incoming data, FIFO overflows, and buffer-full
 * conditions. When data is received, the function reads the bytes from the UART buffer
 * and performs appropriate actions based on the received command (e.g., "PC:On" or "PC:Off").
 *
 * @param[in] pvParameters Not used. Required by FreeRTOS task function signature.
 */
void uart_event_task(void *pvParameters);

/**
 * @brief Callback function to report PC state via UART.
 * * Formats the provided boolean state into a protocol string (e.g., "PC:1\n" or "PC:0\n")
 * and transmits it over the configured UART interface.
 * * @param state The current status of the PC (true for ON/Active, false for OFF/Inactive).
 */
void uart_pc_callback(bool state);


void uart_sendTime(struct tm *timeinfo);

void uart_sendLightState(bool state);