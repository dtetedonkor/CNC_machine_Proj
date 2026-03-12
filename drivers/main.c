/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "../src/core.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
/* 127 chars + NUL terminator for a simple UART shell line. */
#define RX_LINE_SIZE 128u
#define SHELL_TX_TIMEOUT_MS 1000u

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
UART_HandleTypeDef hlpuart1;

/* USER CODE BEGIN PV */
volatile uint8_t rx_byte;
char rx_line[RX_LINE_SIZE];
char cmd_line[RX_LINE_SIZE];
volatile uint8_t rx_index = 0;
volatile uint8_t command_ready = 0;
volatile uint8_t line_overflow = 0;
volatile uint8_t empty_line_ready = 0;
volatile uint8_t rx_restart_error = 0;
volatile uint8_t last_char_was_cr = 0;
core_t cnc_core;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void shell_process_line(const char *line);
void shell_send(const char *s);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_LPUART1_UART_Init();
  /* USER CODE BEGIN 2 */
  core_init(&cnc_core);
  shell_send("\r\nSTM32 shell ready\r\n");
  shell_send("Type help/$ or enter a G-code line\r\n> ");
  if (HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)&rx_byte, 1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    core_poll(&cnc_core);

    uint8_t do_overflow;
    uint8_t do_command;
    uint8_t do_empty_line;
    uint8_t do_rx_restart_error;
    uint8_t prompt_needed = 0;

    __disable_irq();
    do_overflow = line_overflow;
    line_overflow = 0;
    do_command = command_ready;
    if (do_command)
    {
      strncpy(cmd_line, rx_line, RX_LINE_SIZE - 1u);
      cmd_line[RX_LINE_SIZE - 1u] = '\0';
      command_ready = 0;
    }
    do_empty_line = empty_line_ready;
    empty_line_ready = 0;
    do_rx_restart_error = rx_restart_error;
    rx_restart_error = 0;
    __enable_irq();

    if (do_rx_restart_error)
    {
      shell_send("\r\nerror: uart rx restart failed\r\n");
      if (HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)&rx_byte, 1) != HAL_OK)
      {
        __disable_irq();
        rx_restart_error = 1;
        __enable_irq();
      }
      prompt_needed = 1;
    }

    if (do_overflow)
    {
      shell_send("\r\nerror: line overflow\r\n");
      prompt_needed = 1;
    }

    if (do_command)
    {
      shell_process_line(cmd_line);
      prompt_needed = 1;
    }
    else if (do_empty_line)
    {
      prompt_needed = 1;
    }

    if (prompt_needed)
    {
      shell_send("> ");
    }
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

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief LPUART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_LPUART1_UART_Init(void)
{

  /* USER CODE BEGIN LPUART1_Init 0 */

  /* USER CODE END LPUART1_Init 0 */

  /* USER CODE BEGIN LPUART1_Init 1 */

  /* USER CODE END LPUART1_Init 1 */
  hlpuart1.Instance = LPUART1;
  hlpuart1.Init.BaudRate = 115200;
  hlpuart1.Init.WordLength = UART_WORDLENGTH_8B;
  hlpuart1.Init.StopBits = UART_STOPBITS_1;
  hlpuart1.Init.Parity = UART_PARITY_NONE;
  hlpuart1.Init.Mode = UART_MODE_TX_RX;
  hlpuart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  hlpuart1.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  hlpuart1.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  hlpuart1.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  if (HAL_UART_Init(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetTxFifoThreshold(&hlpuart1, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_SetRxFifoThreshold(&hlpuart1, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_UARTEx_DisableFifoMode(&hlpuart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN LPUART1_Init 2 */

  /* USER CODE END LPUART1_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
/* USER CODE BEGIN MX_GPIO_Init_1 */
/* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOA_CLK_ENABLE();

/* USER CODE BEGIN MX_GPIO_Init_2 */
/* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */
void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

  if (huart->Instance == LPUART1)
  {
    PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_LPUART1;
    PeriphClkInit.Lpuart1ClockSelection = RCC_LPUART1CLKSOURCE_PCLK1;
    if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_RCC_LPUART1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitStruct.Pin = GPIO_PIN_2 | GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF12_LPUART1;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    HAL_NVIC_SetPriority(LPUART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(LPUART1_IRQn);
  }
}

void LPUART1_IRQHandler(void)
{
  HAL_UART_IRQHandler(&hlpuart1);
}

void shell_send(const char *s)
{
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)s, strlen(s), SHELL_TX_TIMEOUT_MS);
}

void shell_process_line(const char *line)
{
  if (strcmp(line, "$") == 0)
  {
    char status[64];
    if (core_get_status(&cnc_core, status, sizeof(status)) > 0u)
    {
      shell_send("\r\n");
      shell_send(status);
      shell_send("\r\n");
    }
    shell_send("ok\r\n");
  }
  else if (strcmp(line, "help") == 0)
  {
    shell_send("\r\ncommands: help, $, gcode lines\r\nok\r\n");
  }
  else
  {
    char response[80];
    core_submit_line(&cnc_core, line, response, sizeof(response));
    shell_send("\r\n");
    shell_send(response);
    shell_send("\r\n");
  }
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == LPUART1)
  {
    char c = (char)rx_byte;
    uint8_t is_newline = 0;
    uint8_t skip_char = 0;

    if (c == '\r')
    {
      is_newline = 1;
      last_char_was_cr = 1;
    }
    else if (c == '\n')
    {
      if (last_char_was_cr)
      {
        last_char_was_cr = 0;
        skip_char = 1;
      }
      else
      {
        is_newline = 1;
      }
    }
    else
    {
      last_char_was_cr = 0;
    }

    if (!skip_char && !command_ready)
    {
      if (is_newline)
      {
        if (rx_index > 0)
        {
          if (rx_index < RX_LINE_SIZE)
          {
            rx_line[rx_index] = '\0';
            command_ready = 1;
            rx_index = 0;
          }
          else
          {
            rx_index = 0;
            rx_line[0] = '\0';
            line_overflow = 1;
          }
        }
        else
        {
          empty_line_ready = 1;
        }
      }
      else
      {
        if (rx_index < RX_LINE_SIZE - 1u)
        {
          rx_line[rx_index++] = c;
        }
        else
        {
          rx_index = 0;
          rx_line[0] = '\0';
          line_overflow = 1;
        }
      }
    }

    if (HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)&rx_byte, 1) != HAL_OK)
    {
      rx_restart_error = 1;
    }
  }
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
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
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
