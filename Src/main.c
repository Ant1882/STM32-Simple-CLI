/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  ** This notice applies to any and all portions of this file
  * that are not between comment pairs USER CODE BEGIN and
  * USER CODE END. Other portions of this file, whether 
  * inserted by the user or by software development tools
  * are owned by their respective copyright owners.
  *
  * COPYRIGHT(c) 2018 STMicroelectronics
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
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
#define RX_BUFF_SIZE	8
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
struct UART2_Rx
{
	bool fullFlag;
	bool cmdReady;
	uint8_t count;
	uint8_t buff[RX_BUFF_SIZE];
};

struct UART2_Rx tempBuff;
uint8_t* ptrTempBuff = &tempBuff.buff[0];

char strBuff[128];
bool softReset = false;
bool pinReset = false;

uint8_t goCmd[] = "go";
uint8_t rstCmd[] = "reset";
uint8_t reply[] = "\r\nLED Toggled...\r\n>";
uint8_t prompt[] = "\r\n>";
uint8_t cmdFail[] = "\r\nCommand not recognised...\r\n>";
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void printInfo()
{
	HAL_StatusTypeDef status = HAL_OK;

	uint16_t flashSize = READ_REG(*((uint16_t *)FLASHSIZE_BASE));

	int HalVersion = HAL_GetHalVersion();
	int RevID = HAL_GetREVID();
	int DevID = HAL_GetDEVID();
	int UIDw0 = HAL_GetUIDw0();
	int UIDw1 = HAL_GetUIDw1();
	int UIDw2 = HAL_GetUIDw2();
	int HCLKF = HAL_RCC_GetHCLKFreq();

	if(pinReset)
	{
		sprintf(&strBuff[0], "\r\nPin triggered reset occurred...\r\n");
		status = HAL_UART_Transmit(&huart2, (uint8_t*)&strBuff[0], strlen(strBuff),100);
		assert_param(status == HAL_OK);
	}

	if(softReset)
	{
		sprintf(&strBuff[0], "\r\nSoftware triggered reset occurred...\r\n");
		status = HAL_UART_Transmit(&huart2, (uint8_t*)&strBuff[0], strlen(strBuff),100);
		assert_param(status == HAL_OK);
	}

	sprintf(&strBuff[0], "\r\nSTM32_HAL L0_V%d.%d.%d (RC-%d)\r\n",
		   (HalVersion >> 24),
		   (HalVersion >> 16) & 0xFF,
		   (HalVersion >> 8) & 0xFF,
		    HalVersion & 0xFF);

	// Wait for connection to Tera Term (or whatever) before printing
	HAL_Delay(500);
	// Blocking calls used deliberately
	status = HAL_UART_Transmit(&huart2, (uint8_t*)&strBuff[0], strlen(strBuff),100);
	assert_param(status == HAL_OK);
	sprintf(&strBuff[0], "Flash  : %d Kbytes\r\n", flashSize);
	status = HAL_UART_Transmit(&huart2, (uint8_t*)&strBuff[0], strlen(strBuff),100);
	assert_param(status == HAL_OK);
	sprintf(&strBuff[0], "UID    : %d%d%d\r\n", UIDw2, UIDw1, UIDw0);
	status = HAL_UART_Transmit(&huart2, (uint8_t*)&strBuff[0], strlen(strBuff),100);
	assert_param(status == HAL_OK);
	sprintf(&strBuff[0], "Dev ID : 0x%03x\r\n", DevID);
	status = HAL_UART_Transmit(&huart2, (uint8_t*)&strBuff[0], strlen(strBuff),100);
	assert_param(status == HAL_OK);
	sprintf(&strBuff[0], "Rev ID : 0x%04x\r\n", RevID);
	status = HAL_UART_Transmit(&huart2, (uint8_t*)&strBuff[0], strlen(strBuff),100);
	assert_param(status == HAL_OK);
	sprintf(&strBuff[0], "HCLK   : %d Hz\r\n\n>", HCLKF);
	status = HAL_UART_Transmit(&huart2, (uint8_t*)&strBuff[0], strlen(strBuff),100);
	assert_param(status == HAL_OK);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  HAL_StatusTypeDef status = HAL_OK;

  if(huart->Instance == USART2)
  {
	  // We can accept the character
	  if( (tempBuff.count < RX_BUFF_SIZE) && (tempBuff.fullFlag == false))
	  {
		  // It's the end of a command
		  if(tempBuff.buff[tempBuff.count] == '\r')
		  {
			  status = HAL_UART_Abort(huart);
			  assert_param(status == HAL_OK);

			  // Signal we have a command and reset pointer
			  tempBuff.cmdReady = true;
			  ptrTempBuff = &tempBuff.buff[0];
		  }
		  else // Assume character is good
		  {
			  tempBuff.count += 1;
			  ptrTempBuff++;
			  status = HAL_UART_Receive_IT(huart, ptrTempBuff, sizeof(uint8_t));
			  assert_param(status == HAL_OK);
		  }
	  }
	  else // We can't accept it so start again
	  {
		  status = HAL_UART_Abort(huart);
		  assert_param(status == HAL_OK);

		  // Signal an error condition, reset count and pointer
		  tempBuff.count = 0;
		  tempBuff.fullFlag = true;
		  ptrTempBuff = &tempBuff.buff[0];
	  }
  }
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */
  HAL_StatusTypeDef status = HAL_OK;
  uint8_t response[] = "\r\nbuffer overflow\r\n>";
  tempBuff.fullFlag = false;
  tempBuff.cmdReady = false;
  tempBuff.count = 0;
  memset(&tempBuff.buff, 0x00, sizeof(tempBuff.buff));

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */
  // See whether we have been reset and how it occurred
  if __HAL_RCC_GET_FLAG(RCC_FLAG_SFTRST)   //TODO:Add more reset reasons
  {
    softReset = true;
  }
  if __HAL_RCC_GET_FLAG(RCC_FLAG_PINRST)
  {
	pinReset = true;
  }

  // Clear the reset flags
  __HAL_RCC_CLEAR_RESET_FLAGS();

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_USART2_UART_Init();
  /* USER CODE BEGIN 2 */

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  // Begin reception
  status = HAL_UART_Receive_IT(&huart2, &tempBuff.buff[0], sizeof(uint8_t));
  assert_param(status == HAL_OK);

  printInfo();

  while (1)
  {
    /* USER CODE END WHILE */
	HAL_Delay(5);

	if(tempBuff.fullFlag == true)
	{
		// We ran out of buffer before getting a command!
		status = HAL_UART_Abort(&huart2);
		assert_param(status == HAL_OK);

		// Clear full flag, count, and buffer
		tempBuff.count = 0;
		tempBuff.fullFlag = false;
		memset(&tempBuff.buff, 0x00, sizeof(tempBuff.buff));

		// Send error message & enable reception again
		status = HAL_UART_Transmit_IT(&huart2, &response[0], sizeof(response));
		assert_param(status == HAL_OK);
		status = HAL_UART_Receive_IT(&huart2, &tempBuff.buff[0], sizeof(uint8_t));
		assert_param(status == HAL_OK);
	}
	if(tempBuff.cmdReady == true)
	{
		// We know the count so turn the end to a null terminator
		tempBuff.buff[tempBuff.count] = '\0';

		if(tempBuff.count > 0)
		{
			// We have at least one character plus CR, so process
			if(strcmp((const char*)goCmd, (const char*)tempBuff.buff) == 0)
			{
				// Do something and give a response
				HAL_GPIO_TogglePin(LD2_GPIO_Port, LD2_Pin);
				status = HAL_UART_Transmit_IT(&huart2, &reply[0], sizeof(reply));
				assert_param(status == HAL_OK);
			}
			else if(strcmp((const char*)rstCmd, (const char*)tempBuff.buff) == 0)
			{
				sprintf(&strBuff[0], "\r\nResetting MCU...\r\n");
				// Deliberately blocking so we don't reset half way through the message
				status = HAL_UART_Transmit(&huart2, (uint8_t*)&strBuff[0], strlen(strBuff),100);
				assert_param(status == HAL_OK);
				NVIC_SystemReset();
			}
			else
			{
				// Inform that the command is not recognised
				status = HAL_UART_Transmit_IT(&huart2, &cmdFail[0], sizeof(cmdFail));
				assert_param(status == HAL_OK);
			}
		}
		else
		{
			// Nothing to process, so just give a prompt
			status = HAL_UART_Transmit_IT(&huart2, &prompt[0], sizeof(prompt));
			assert_param(status == HAL_OK);
		}

		// Clear ready flag, count, and buffer
		tempBuff.count = 0;
		tempBuff.cmdReady = false;
		memset(&tempBuff.buff, 0x00, sizeof(tempBuff.buff));

		// Enable reception again
		status = HAL_UART_Receive_IT(&huart2, &tempBuff.buff[0], sizeof(uint8_t));
		assert_param(status == HAL_OK);
	}
    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  /**Configure the main internal regulator output voltage 
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLLMUL_8;
  RCC_OscInitStruct.PLL.PLLDIV = RCC_PLLDIV_2;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }
  /**Initializes the CPU, AHB and APB busses clocks 
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK)
  {
    Error_Handler();
  }
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART2;
  PeriphClkInit.Usart2ClockSelection = RCC_USART2CLKSOURCE_PCLK1;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */

  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{ 
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
	sprintf(&strBuff[0], "\r\nAssert Failed: file %s on line %d\r\n", file, (int)line);
	HAL_UART_Transmit(&huart2, (uint8_t*)&strBuff[0], strlen(strBuff),100);
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
