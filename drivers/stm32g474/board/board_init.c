#include "board_init.h"

#include "board_pins.h"

#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>

void board_init_clocks(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOC);
}

void board_init_gpio(void) {
    const uint16_t step_pins = BOARD_STEP_PIN_X | BOARD_STEP_PIN_Y | BOARD_STEP_PIN_Z | BOARD_STEP_PIN_A;
    const uint16_t dir_pins = BOARD_DIR_PIN_X | BOARD_DIR_PIN_Y | BOARD_DIR_PIN_Z | BOARD_DIR_PIN_A;

    gpio_mode_setup(BOARD_STEP_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, step_pins);
    gpio_mode_setup(BOARD_DIR_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, dir_pins);
    gpio_mode_setup(BOARD_DRV_EN_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, BOARD_DRV_EN_PIN);

    gpio_clear(BOARD_STEP_PORT, step_pins);
    gpio_clear(BOARD_DIR_PORT, dir_pins);

#if BOARD_DRV_EN_ACTIVE_LOW
    gpio_set(BOARD_DRV_EN_PORT, BOARD_DRV_EN_PIN);
#else
    gpio_clear(BOARD_DRV_EN_PORT, BOARD_DRV_EN_PIN);
#endif

    gpio_mode_setup(
        BOARD_INPUT_PORT,
        GPIO_MODE_INPUT,
        GPIO_PUPD_PULLUP,
        BOARD_INPUT_ESTOP_PIN | BOARD_INPUT_LIMIT_X_PIN | BOARD_INPUT_LIMIT_Y_PIN |
            BOARD_INPUT_LIMIT_Z_PIN | BOARD_INPUT_HAND_PIN);
}

void board_init_uart_pins(void) {
    gpio_mode_setup(BOARD_USART_GPIO_PORT, GPIO_MODE_AF, GPIO_PUPD_NONE, BOARD_USART_TX_PIN | BOARD_USART_RX_PIN);
    gpio_set_af(BOARD_USART_GPIO_PORT, BOARD_USART_AF, BOARD_USART_TX_PIN | BOARD_USART_RX_PIN);
}
