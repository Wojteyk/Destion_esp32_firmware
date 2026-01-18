#pragma once

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

extern bool relay_state;

/**
 * @brief Callback function to control the light state based on received Firebase data.
 * * This function parses the incoming JSON payload (supporting "true"/"1" for ON and "false"/"0" for OFF)
 * and triggers the corresponding UART command to the connected MCU.
 * * @note "null" payloads (keep-alive messages) are safely ignored to prevent errors.
 * * @param json_payload The raw string data received from the Firebase stream.
 */
void set_light_state(const char* json_payload);

/**
 * @brief Sets the relay state based on a JSON payload.
 *
 * This function parses the given JSON payload to determine
 * the desired relay state (e.g., ON or OFF) and updates
 * the GPIO output accordingly.
 *
 * @param json_payload A JSON-formatted string containing the relay state information.
 *             
 */
void set_relay_state(const char* json_payload);

/**
 * @brief Relay hardware initialization.
 *
 * Configures the relay GPIO pin as output and sets initial state to OFF.
 */
void relay_init(void);

/**
 * @brief Initialize PC switch (button) input.
 *
 * Configures the button GPIO pin with interrupt on falling edge,
 * creates the event queue, and attaches the ISR handler.
 */
void pc_switch_init(void);

/**
 * @brief FreeRTOS task to handle button press events.
 *
 * Listens to the button GPIO event queue, applies debouncing,
 * toggles the relay state, and updates Firebase.
 *
 * @param pvParameters Task parameters (unused)
 */
void button_handler_task(void* pvParameters);

/**
 * @brief FreeRTOS task responsible for processing window sensor events.
 *
 * The task waits for GPIO interrupt events sent through window_evt_queue.
 * On each event, it reads the current window state, compares it with the previous one
 * and triggers an action when a change is detected.
 *
 * It logs window open/close events and publishes updates to Firebase.
 *
 * @param pvParameters Unused task parameter.
 */
void window_task(void* pvParameters);

/**
 * @brief Initialize the window sensor GPIO and interrupt handling.
 *
 * Configures the window sensor pin as an input with edge-triggered interrupts.
 * Creates a queue for passing GPIO events to the window task.
 * Installs the ISR service and attaches the interrupt handler to the pin.
 *
 * This function must be called before creating the window_task().
 */
void window_init();