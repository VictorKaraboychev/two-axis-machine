/**
 ******************************************************************************
 * File Name          : main.c
 * Date               : 09/10/2014 11:13:03
 * Description        : Main program body
 ******************************************************************************
 *
 * COPYRIGHT(c) 2014 STMicroelectronics
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *   1. Redistributions of source code must retain the above copyright notice,
 *      this list of conditions and the following disclaimer.
 *   2. Redistributions in binary form must reproduce the above copyright notice,
 *      this list of conditions and the following disclaimer in the documentation
 *      and/or other materials provided with the distribution.
 *   3. Neither the name of STMicroelectronics nor the names of its contributors
 *      may be used to endorse or promote products derived from this software
 *      without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 ******************************************************************************
 */

// test git

#include "example.h"
#include "example_usart.h"

#include "stm32f4xx.h"
#include "stm32f4xx_nucleo.h"
#include "system_stm32f4xx.h"
#include "stm32f4xx_hal_gpio.h"
#include "stm32f4xx_hal_rcc.h"

#include <stdbool.h>

/**
 * @defgroup   MotionControl
 * @{
 */

/**
 * @addtogroup BSP
 * @{
 */

/**
 * @}
 */
/* End of BSP */

/**
 * @addtogroup MicrosteppingMotor_Example
 * @{
 */

/**
 * @defgroup   ExampleTypes
 * @{
 */

//------- DEFINES ---------------------------------------------------

// #define MICROSTEPPING_MOTOR_EXAMPLE        //!< Uncomment to performe the standalone example
#define MICROSTEPPING_MOTOR_USART_EXAMPLE //!< Uncomment to performe the USART example
#if ((defined(MICROSTEPPING_MOTOR_EXAMPLE)) && (defined(MICROSTEPPING_MOTOR_USART_EXAMPLE)))
#error "Please select an option only!"
#elif ((!defined(MICROSTEPPING_MOTOR_EXAMPLE)) && (!defined(MICROSTEPPING_MOTOR_USART_EXAMPLE)))
#error "Please select an option!"
#endif
#if (defined(MICROSTEPPING_MOTOR_USART_EXAMPLE) && (!defined(NUCLEO_USE_USART)))
#error "Please define "NUCLEO_USE_USART" in "stm32fxxx_x-nucleo-ihm02a1.h"!"
#endif

//------- GLOBAL VARIABLES ------------------------------------------

//------- MAIN FUNCTION ---------------------------------------------

static void MX_GPIO_Init(void)
{
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_SYSCFG_CLK_ENABLE();

	// GPIO_InitTypeDef GPIO_InitStruct;

	// // Configure GPIO pin PA8
	// GPIO_InitStruct.Pin = GPIO_PIN_8;
	// GPIO_InitStruct.Mode = GPIO_MODE_IT_RISING;
	// GPIO_InitStruct.Pull = GPIO_NOPULL;
	// GPIO_InitStruct.Speed = GPIO_SPEED_HIGH;
	// HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

	// // EXTI interrupt init
	// HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
	// HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

	GPIO_InitTypeDef GPIO_InitStructure;

	/* Poll GPIO Init PA8 */
	__GPIOA_CLK_ENABLE();
	GPIO_InitStructure.Pin = GPIO_PIN_8;
	GPIO_InitStructure.Mode = GPIO_MODE_INPUT;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
	__GPIOA_CLK_DISABLE();

	/* LED GPIO Init PA9 */
	__GPIOA_CLK_ENABLE();
	GPIO_InitStructure.Pin = GPIO_PIN_9;
	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;
	GPIO_InitStructure.Pull = GPIO_PULLUP;
	GPIO_InitStructure.Speed = GPIO_SPEED_HIGH;
	HAL_GPIO_Init(GPIOA, &GPIO_InitStructure);
	__GPIOA_CLK_DISABLE();
}

/**
 * @}
 */
/* End of ExampleTypes */

/**
 * @brief The FW main module
 */
int main(void)
{
	/* NUCLEO board initialization */
	NUCLEO_Board_Init();

	/* GPIO initialization */
	MX_GPIO_Init();

	/* X-NUCLEO-IHM02A1 initialization */
	BSP_Init();

#ifdef NUCLEO_USE_USART
	/* Transmit the initial message to the PC via UART */
	USART_TxWelcomeMessage();
	USART_Transmit(&huart2, " X-CUBE-SPN2 v1.0.0\n\r");
#endif

#if defined(MICROSTEPPING_MOTOR_EXAMPLE)
	/* Perform a batch commands for X-NUCLEO-IHM02A1 */
	MicrosteppingMotor_Example_01();

	/* Infinite loop */
	while (1)
		;
#elif defined(MICROSTEPPING_MOTOR_USART_EXAMPLE)
	/* Fill the L6470_DaisyChainMnemonic structure */
	Fill_L6470_DaisyChainMnemonic();

	/*Initialize the motor parameters */
	Motor_Param_Reg_Init();

	bool last_state = false;

	/* Infinite loop */
	while (1)
	{
		/* Check if any Application Command for L6470 has been entered by USART */
		// USART_CheckAppCmd();

		bool current_state = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_8);

		if (current_state && !last_state) // check for rising edge
		{
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_SET);
			HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
		}
		// else if (!current_state && last_state) // check for falling edge
		// {
		// 	HAL_GPIO_WritePin(GPIOA, GPIO_PIN_9, GPIO_PIN_RESET);
		// }

		last_state = current_state;
	}
#endif
}

#ifdef USE_FULL_ASSERT

/**
 * @brief Reports the name of the source file and the source line number
 * where the assert_param error has occurred.
 * @param file: pointer to the source file name
 * @param line: assert_param error line source number
 * @retval None
 */
void assert_failed(uint8_t *file, uint32_t line)
{
	/* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
	  ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	/* USER CODE END 6 */
}

#endif

/**
 * @}
 */
/* End of MicrosteppingMotor_Example */

/**
 * @}
 */
/* End of MotionControl */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
