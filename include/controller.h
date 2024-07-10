#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"

#define L6470_X 0
#define L6470_Y 1

#define DEBOUNCE_DELAY 5

#define ANALOG_DEBOUNCE_DELAY 500
#define ANALOG_DEBOUNCE_TOLERANCE 2

#define MAX_VELOCITY 15610.0f

typedef struct
{
	GPIO_TypeDef *GPIOx;	   // GPIO port
	uint16_t GPIO_Pin;		   // GPIO pin
	uint32_t lastDebounceTime; // last time the output pin was toggled
	GPIO_PinState lastState;   // last state of the output pin
} Debounce_t;

typedef struct
{
	float lastState;		   // last state of the analog input
	uint32_t lastDebounceTime; // last time the analog input was read
} DebounceAnalog_t;

/**
 * @brief initializes the controller pins
 */
void ControllerInit();

/**
 * @brief main controller loop
 */
void ControllerMain();

#endif // CONTROLLER_H