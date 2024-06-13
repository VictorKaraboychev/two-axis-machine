#ifndef CONTROLLER_H
#define CONTROLLER_H

#include "stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"

#include <L6470.h>

#include <stdbool.h>

#define L6470_X 0
#define L6470_Y 1

#define DEBOUNCE_DELAY 5

typedef struct
{
	GPIO_TypeDef *GPIOx;
	uint16_t GPIO_Pin;
	uint32_t lastDebounceTime;
	GPIO_PinState lastState;
} Debounce_t;

void ControllerInit();
void ControllerMain();

int positionX;
int velocityX;

int positionY;
int velocityY;

#endif // CONTROLLER_H