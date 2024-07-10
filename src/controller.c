#include "controller.h"

#include <L6470.h>
#include <stdbool.h>

// ------------------ Global Variables ------------------

// initialize the switch debounce delay and directions of each limit switch
Debounce_t switchPosX = {GPIOA, GPIO_PIN_8, 0, GPIO_PIN_SET};
Debounce_t switchNegX = {GPIOA, GPIO_PIN_9, 0, GPIO_PIN_SET};

Debounce_t switchPosY = {GPIOB, GPIO_PIN_10, 0, GPIO_PIN_SET};
Debounce_t switchNegY = {GPIOB, GPIO_PIN_4, 0, GPIO_PIN_SET};

// declare adc handler for reading analog values
ADC_HandleTypeDef hadc1;

// initialize the potentiometer debounce delay and last state of each potentiometer
DebounceAnalog_t potX = {0.0f, 0};
DebounceAnalog_t potY = {0.0f, 0};

// target and current velocity values for the motors
int targetVelocityX;
int targetVelocityY;

int currentVelocityX;
int currentVelocityY;

// ------------------ Private Functions ------------------

/**
 * @brief returns the absolute value of an integer
 *
 * @param x the integer to get the absolute value of
 * @return int the absolute value of x
 */
int abs(int x)
{
	if (x < 0)
	{
		return -x;
	}
	return x;
}

/**
 * @brief Debounces a limit switch
 *
 * @param switchState the state of the limit switch
 * @param delay the debounce delay
 * @return previous state of the switch if the debounce delay has not been reached, otherwise the current state
 */
bool debounceLimitSwitch(Debounce_t *switchState, uint32_t delay)
{
	// Read the current state of the switch
	uint32_t currentTime = HAL_GetTick();
	GPIO_PinState currentState = HAL_GPIO_ReadPin(switchState->GPIOx, switchState->GPIO_Pin);

	if (currentState != switchState->lastState)
	{
		// Reset the debounce timer
		switchState->lastDebounceTime = currentTime;
		switchState->lastState = currentState;
	}

	if ((currentTime - switchState->lastDebounceTime) >= delay)
	{
		return currentState;
	}

	return switchState->lastState;
}

/**
 * @brief Reads an analog value from the ADC
 *
 * @param channel the channel to read from
 * @return float the analog value from 0 to 1
 */
float readAnalog(uint32_t channel)
{
	ADC_ChannelConfTypeDef sConfig;
	sConfig.Channel = channel;
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_15CYCLES;

	HAL_ADC_ConfigChannel(&hadc1, &sConfig);

	HAL_ADC_Start(&hadc1);
	if (HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK)
	{
		return (float)HAL_ADC_GetValue(&hadc1) / 63.0f;
	}
	return 0.0f; // Return 0 if ADC read fails
}

/**
 * @brief Debounces an analog input
 *
 * @param switchState the state of the analog input
 * @param delay the debounce delay
 * @param analogValue the analog value to debounce
 * @return previous state of the analog input if the debounce delay has not been reached, otherwise the current state
 */
float debounceAnalogInput(DebounceAnalog_t *switchState, uint32_t delay, float analogValue)
{
	// Read the current state of the switch
	uint32_t currentTime = HAL_GetTick();
	float currentState = analogValue;

	if (abs((int)(currentState - switchState->lastState)) > ANALOG_DEBOUNCE_TOLERANCE)
	{
		// Reset the debounce timer
		switchState->lastDebounceTime = currentTime;
		switchState->lastState = currentState;
	}

	if ((currentTime - switchState->lastDebounceTime) >= delay)
	{
		return currentState;
	}

	return switchState->lastState;
}

// ------------------ Public Functions ------------------

void ControllerInit()
{
	// Enable GPIO clock
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	// Configure GPIO pins for limit switches
	GPIO_InitTypeDef GPIO_InitStruct;

	GPIO_InitStruct.Pin = GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_4 | GPIO_PIN_10;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
	GPIO_InitStruct.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

	GPIO_InitStruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
	GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
	GPIO_InitStruct.Pull = GPIO_NOPULL;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// Common analog config
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
	hadc1.Init.Resolution = ADC_RESOLUTION_6B;
	// hadc1.Init.ScanConvMode = DISABLE;
	// hadc1.Init.ContinuousConvMode = DISABLE;
	// hadc1.Init.DiscontinuousConvMode = DISABLE;
	// hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	// hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	// hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	// hadc1.Init.NbrOfConversion = 1;
	// hadc1.Init.DMAContinuousRequests = DISABLE;
	// hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

	HAL_ADC_Init(&hadc1);

	targetVelocityX = 0;
	targetVelocityY = 0;

	currentVelocityX = 0;
	currentVelocityY = 0;
}

void ControllerMain()
{
	// debounce limit switches to prevent false positives
	bool limitSwitchPosX = debounceLimitSwitch(&switchPosX, DEBOUNCE_DELAY);
	bool limitSwitchNegX = debounceLimitSwitch(&switchNegX, DEBOUNCE_DELAY);

	bool limitSwitchPosY = debounceLimitSwitch(&switchPosY, DEBOUNCE_DELAY);
	bool limitSwitchNegY = debounceLimitSwitch(&switchNegY, DEBOUNCE_DELAY);

	// obtain analog values from the potentiometers
	float potX = readAnalog(ADC_CHANNEL_0);
	float potY = readAnalog(ADC_CHANNEL_1);

	// Calculate target velocity based on potentiometer position and deadzone of 20%
	targetVelocityX = (int)(2.0f * (potX - 0.5f) * MAX_VELOCITY * (potX < 0.4f || potX > 0.6f));
	targetVelocityY = (int)(2.0f * (potY - 0.5f) * MAX_VELOCITY * (potY < 0.4f || potY > 0.6f));

	// X LIMIT SWITCH CHECKS
	if (limitSwitchPosX && targetVelocityX > 0) // Hit positive X limit switch
	{
		targetVelocityX = 0;

		printf("Hit positive X limit switch\n\r");
	}
	else if (limitSwitchNegX && targetVelocityX < 0) // Hit negative X limit switch
	{
		targetVelocityX = 0;

		printf("Hit negative X limit switch\n\r");
	}

	// Y LIMIT SWITCHES CHECKS
	if (limitSwitchPosY && targetVelocityY > 0) // Hit positive Y limit switch
	{
		targetVelocityY = 0;

		printf("Hit positive Y limit switch\n\r");
	}
	else if (limitSwitchNegY && targetVelocityY < 0) // Hit negative Y limit switch
	{
		targetVelocityY = 0;

		printf("Hit negative Y limit switch\n\r");
	}

	// If both limit switches are hit in the same direction, stop the motor
	if ((limitSwitchPosX && limitSwitchNegX) || (limitSwitchPosY && limitSwitchNegY))
	{
		targetVelocityX = 0.0f;
		targetVelocityY = 0.0f;

		// printf("Hit both limit switches\n");
	}

	// Run the X motor if the velocity has changed
	if (currentVelocityX != targetVelocityX)
	{
		// printf("Running X motor\n\r");

		if (targetVelocityX == 0)
		{
			L6470_HardStop(L6470_X);
		}
		else
		{
			L6470_Run(L6470_X, targetVelocityX > 0, abs(targetVelocityX));
		}

		currentVelocityX = targetVelocityX;
	}

	// Run the Y motor if the velocity has changed
	if (currentVelocityY != targetVelocityY)
	{
		// printf("Running Y motor\n\r");

		if (targetVelocityY == 0)
		{
			L6470_HardStop(L6470_Y);
		}
		else
		{
			L6470_Run(L6470_Y, targetVelocityY > 0, abs(targetVelocityY));
		}

		currentVelocityY = targetVelocityY;
	}
}