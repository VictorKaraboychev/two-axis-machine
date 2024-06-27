#include "controller.h"

int sign(float x)
{
	if (x > 0)
	{
		return 1;
	}
	else if (x < 0)
	{
		return -1;
	}
	return 0;
}

int abs(int x)
{
	if (x < 0)
	{
		return -x;
	}
	return x;
}

Debounce_t switchPosX = {GPIOA, GPIO_PIN_8, 0, GPIO_PIN_SET};
Debounce_t switchNegX = {GPIOA, GPIO_PIN_9, 0, GPIO_PIN_SET};

Debounce_t switchPosY = {GPIOB, GPIO_PIN_10, 0, GPIO_PIN_SET};
Debounce_t switchNegY = {GPIOB, GPIO_PIN_4, 0, GPIO_PIN_SET};

bool debounceLimitSwitch(Debounce_t *switchState, uint32_t delay)
{
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

	return false;
}

ADC_HandleTypeDef hadc1;

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
	bool limitSwitchPosX = debounceLimitSwitch(&switchPosX, DEBOUNCE_DELAY);
	bool limitSwitchNegX = debounceLimitSwitch(&switchNegX, DEBOUNCE_DELAY);

	bool limitSwitchPosY = debounceLimitSwitch(&switchPosY, DEBOUNCE_DELAY);
	bool limitSwitchNegY = debounceLimitSwitch(&switchNegY, DEBOUNCE_DELAY);

	float potX = readAnalog(ADC_CHANNEL_0);
	float potY = readAnalog(ADC_CHANNEL_1);

	// Calculate target velocity based on potentiometer position and deadzone of 20%
	targetVelocityX = 2.0f * (potX - 0.5f) * MAX_VELOCITY * (potX < 0.4f || potX > 0.5f);
	targetVelocityY = 2.0f * (potY - 0.5f) * MAX_VELOCITY * (potY < 0.4f || potY > 0.5f);

	// // print targetVelocityX and targetVelocityY
	// char bufferX[100];
	// gcvt((float)targetVelocityX, 6, bufferX);

	// char bufferY[100];
	// gcvt((float)targetVelocityY, 6, bufferY);

	// printf("Velocity Values: ");
	// printf(bufferX);
	// printf(" ");
	// printf(bufferY);
	// printf("\n");

	if (limitSwitchPosX && targetVelocityX > 0.0f) // Hit positive X limit switch
	{
		targetVelocityX = 0;

		printf("Hit positive X limit switch\n");
	}
	else if (limitSwitchNegX && targetVelocityX < 0.0f) // Hit negative X limit switch
	{
		targetPositionX = 0;
		targetVelocityX = 0;

		printf("Hit negative X limit switch\n");
	}

	// Run the X motor if the velocity has changed
	if (abs(currentVelocityX - targetVelocityX) > 1000)
	{
		printf("Running X motor\n");

		L6470_Run(L6470_X, sign(targetVelocityX), abs(targetVelocityX));
		currentVelocityX = targetVelocityX;
	}

	if (limitSwitchPosY && targetVelocityY > 0.0f) // Hit positive Y limit switch
	{
		targetVelocityY = 0;

		printf("Hit positive Y limit switch\n");
	}
	else if (limitSwitchNegY && targetVelocityY < 0.0f) // Hit negative Y limit switch
	{
		targetPositionY = 0;
		targetVelocityY = 0;

		printf("Hit negative Y limit switch\n");
	}

	// Run the Y motor if the velocity has changed
	if (abs(currentVelocityY - targetVelocityY) > 1000)
	{
		printf("Running Y motor\n");

		L6470_Run(L6470_Y, sign(targetVelocityY), abs(targetVelocityY));
		currentVelocityY = targetVelocityY;
	}

	// If both limit switches are hit in the same direction, stop the motor
	if ((limitSwitchPosX && limitSwitchNegX) || (limitSwitchPosY && limitSwitchNegY))
	{
		targetVelocityX = 0.0f;
		targetVelocityY = 0.0f;

		L6470_HardStop(L6470_X);
		L6470_HardStop(L6470_Y);

		// printf("Hit both limit switches\n");
	}
}