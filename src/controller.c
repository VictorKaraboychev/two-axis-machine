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

int currentVelocityX = 0;
int currentVelocityY = 0;

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
		// If the pin state has been stable for the delay period
		if (currentState == switchState->lastState)
		{
			return currentState;
		}
	}

	return false;
}

ADC_HandleTypeDef hadc1;

float readAnalog(void)
{
	if (HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY) == HAL_OK)
	{
		uint32_t adc_value = HAL_ADC_GetValue(&hadc1);
		return (float)(adc_value / 2047.5f) - 1.0f; // Scale 12-bit ADC value to -1 - 1 range
	}
	return 0.0f; // Return 0 if ADC read fails
}

void ControllerInit()
{
	// Enable GPIO clock
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	// Configure GPIO pins for limit switches
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.Pin = GPIO_PIN_8 | GPIO_PIN_9;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);

	GPIO_InitStructure.Pin = GPIO_PIN_4 | GPIO_PIN_10;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	HAL_GPIO_Init(GPIOB, &GPIO_InitStructure);

	ADC_ChannelConfTypeDef sConfig;

	// Common analog config
	hadc1.Instance = ADC1;
	hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV2;
	hadc1.Init.Resolution = ADC_RESOLUTION_12B;
	hadc1.Init.ScanConvMode = DISABLE;
	hadc1.Init.ContinuousConvMode = DISABLE;
	hadc1.Init.DiscontinuousConvMode = DISABLE;
	hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
	hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
	hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
	hadc1.Init.NbrOfConversion = 1;
	hadc1.Init.DMAContinuousRequests = DISABLE;
	hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;

	// Configure for the selected ADC regular channel to be converted.
	sConfig.Channel = ADC_CHANNEL_0; // Adjust according to the pin you're using
	sConfig.Rank = 1;
	sConfig.SamplingTime = ADC_SAMPLETIME_3CYCLES;

	HAL_ADC_Start(&hadc1);

	velocityX = 1000;
	velocityY = 1000;
}

void ControllerMain()
{
	bool limitSwitchPosX = debounceLimitSwitch(&switchPosX, DEBOUNCE_DELAY);
	bool limitSwitchNegX = debounceLimitSwitch(&switchNegX, DEBOUNCE_DELAY);

	bool limitSwitchPosY = debounceLimitSwitch(&switchPosY, DEBOUNCE_DELAY);
	bool limitSwitchNegY = debounceLimitSwitch(&switchNegY, DEBOUNCE_DELAY);

	// float analogValue = readAnalog();

	// printf("Analog value: %f\n", analogValue);

	if (limitSwitchPosX && velocityX > 0.0f) // Hit positive X limit switch
	{
		velocityX = -1000;

		USART_Transmit(&huart2, "Hit positive X limit switch\n");
	}
	else if (limitSwitchNegX && velocityX < 0.0f) // Hit negative X limit switch
	{
		positionX = 0;
		velocityX = 1000;

		USART_Transmit(&huart2, "Hit negative X limit switch\n");
	}

	// Run the X motor if the velocity has changed
	if (currentVelocityX != velocityX)
	{
		USART_Transmit(&huart2, "Running X motor\n");

		L6470_Run(L6470_X, sign(velocityX), abs(velocityX));
		currentVelocityX = velocityX;
	}

	if (limitSwitchPosY && velocityY > 0.0f) // Hit positive Y limit switch
	{
		velocityY = -1000;

		USART_Transmit(&huart2, "Hit positive Y limit switch\n");
	}
	else if (limitSwitchNegY && velocityY < 0.0f) // Hit negative Y limit switch
	{
		positionY = 0;
		velocityY = 1000;

		USART_Transmit(&huart2, "Hit negative Y limit switch\n");
	}

	// Run the Y motor if the velocity has changed
	if (currentVelocityY != velocityY)
	{
		USART_Transmit(&huart2, "Running Y motor\n");

		L6470_Run(L6470_Y, sign(velocityY), abs(velocityY));
		currentVelocityY = velocityY;
	}

	// If both limit switches are hit in the same direction, stop the motor
	if ((limitSwitchPosX && limitSwitchNegX) || (limitSwitchPosY && limitSwitchNegY))
	{
		velocityX = 0.0f;
		L6470_HardStop(L6470_X);
		velocityY = 0.0f;
		L6470_HardStop(L6470_Y);

		USART_Transmit(&huart2, "Hit both limit switches\n");
	}
}