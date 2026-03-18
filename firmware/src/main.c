/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body (STM32G4 / LPUART1)
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
#include <stdio.h>
#include "firmware_gcode_streamer.h"

fw_gcode_streamer_t g_streamer;
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
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
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_LPUART1_UART_Init(void);
/* USER CODE BEGIN PFP */
void shell_process_line(const char *line);
void shell_send(const char *s);

static void boot_print_banner_and_checks(void);
static uint8_t boot_check_irq_ok(char *err, size_t err_sz);
static uint8_t boot_check_usart_ok(char *err, size_t err_sz);
static uint8_t boot_check_rxit_priming_ok(char *err, size_t err_sz);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
void App_Init(void)
{
  fw_gcode_streamer_init(&g_streamer);
}

void shell_send(const char *s)
{
  HAL_UART_Transmit(&hlpuart1, (uint8_t *)s, (uint16_t)strlen(s), SHELL_TX_TIMEOUT_MS);
}

static void shell_send_line(const char *s)
{
  shell_send(s);
  shell_send("\r\n");
}

/* 1) IRQ/NVIC check */
static uint8_t boot_check_irq_ok(char *err, size_t err_sz)
{
  if (err_sz > 0) err[0] = '\0';

  if (NVIC_GetEnableIRQ(LPUART1_IRQn) == 0U)
  {
    snprintf(err, err_sz, "IRQ NOT OK: LPUART1_IRQn not enabled");
    return 0;
  }

  /* Priority check is optional; just report it (not a fail). */
  return 1;
}

/* 2) USART peripheral config/clock checks */
static uint8_t boot_check_usart_ok(char *err, size_t err_sz)
{
  if (err_sz > 0) err[0] = '\0';

  if (hlpuart1.Instance != LPUART1)
  {
    snprintf(err, err_sz, "USART NOT OK: hlpuart1.Instance != LPUART1");
    return 0;
  }

  if (__HAL_RCC_LPUART1_IS_CLK_DISABLED())
  {
    snprintf(err, err_sz, "USART NOT OK: LPUART1 clock disabled");
    return 0;
  }

  /* On STM32G4, LPUART1 TX/RX are on GPIOA on many configs; GPIOA clock is needed.
   * If you use a different port, adjust this check accordingly.
   */
  if (__HAL_RCC_GPIOA_IS_CLK_DISABLED())
  {
    snprintf(err, err_sz, "GPIO NOT OK: GPIOA clock disabled");
    return 0;
  }

  /* Check that HAL considers the peripheral initialized */
  if (hlpuart1.gState == HAL_UART_STATE_RESET)
  {
    snprintf(err, err_sz, "USART NOT OK: HAL UART state RESET");
    return 0;
  }

  /* Optional: check USART enabled at peripheral level */
  if ((READ_BIT(hlpuart1.Instance->CR1, USART_CR1_UE) == 0U))
  {
    snprintf(err, err_sz, "USART NOT OK: UE bit not set (USART disabled)");
    return 0;
  }

  return 1;
}

/* 3) RX interrupt priming check (verifies HAL_UART_Receive_IT can be started) */
static uint8_t boot_check_rxit_priming_ok(char *err, size_t err_sz)
{
  if (err_sz > 0) err[0] = '\0';

  /* Try to start RX IT once, then immediately abort it.
   * This verifies the HAL path can arm the interrupt without failing.
   */
  if (HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)&rx_byte, 1) != HAL_OK)
  {
    snprintf(err, err_sz, "RXIT NOT OK: HAL_UART_Receive_IT failed to start");
    return 0;
  }

  /* Abort the receive so main() can start it cleanly afterwards. */
  (void)HAL_UART_AbortReceive_IT(&hlpuart1);

  return 1;
}

static void boot_print_banner_and_checks(void)
{
  char err[96];

  shell_send_line("");
  shell_send_line("Signature engravers v1.0");
  shell_send_line("------------------------");

  /* IRQ check */
  if (boot_check_irq_ok(err, sizeof(err)))
  {
    shell_send_line("IRQ OK");
  }
  else
  {
    shell_send_line(err);
  }

  /* USART check */
  if (boot_check_usart_ok(err, sizeof(err)))
  {
    shell_send_line("USART OK");
  }
  else
  {
    shell_send_line(err);
  }

  /* RX IT priming check */
  if (boot_check_rxit_priming_ok(err, sizeof(err)))
  {
    shell_send_line("RXIT OK");
  }
  else
  {
    shell_send_line(err);
  }

  shell_send_line("BOOT OK");
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_LPUART1_UART_Init();

  /* USER CODE BEGIN 2 */
  boot_print_banner_and_checks();

  App_Init();

  /* Start RX interrupt for real (1 byte at a time) */
  if (HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)&rx_byte, 1) != HAL_OK)
  {
    shell_send_line("FATAL: HAL_UART_Receive_IT start failed");
    Error_Handler();
  }
  /* USER CODE END 2 */

  while (1)
  {
    fw_gcode_streamer_poll(&g_streamer);

    uint8_t out;
    while (fw_gcode_streamer_tx_pop(&g_streamer, &out))
    {
      HAL_UART_Transmit(&hlpuart1, &out, 1, 10);
    }

    if (rx_restart_error)
    {
      rx_restart_error = 0;
    }
  }
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

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
}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  __HAL_RCC_GPIOA_CLK_ENABLE();
}

/* USER CODE BEGIN 4 */
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
  if (huart->Instance == LPUART1)
  {
    /* Treat Enter (CR) as newline (LF) for the streamer */
    if (rx_byte == '\r') rx_byte = '\n';

    fw_gcode_streamer_rx_bytes(&g_streamer, (uint8_t *)&rx_byte, 1);

    if (HAL_UART_Receive_IT(&hlpuart1, (uint8_t *)&rx_byte, 1) != HAL_OK)
    {
      rx_restart_error = 1;
    }
  }
}
/* USER CODE END 4 */

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}