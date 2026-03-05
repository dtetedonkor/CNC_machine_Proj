#include "drv_gpio.h"

#include "../board/board_init.h"
#include "../board/board_pins.h"

#include <libopencm3/stm32/gpio.h>

static volatile uint32_t g_pending_step_mask = 0;

static uint16_t axis_step_pin(uint8_t axis) {
    switch (axis) {
        case 0:
            return BOARD_STEP_PIN_X;
        case 1:
            return BOARD_STEP_PIN_Y;
        case 2:
            return BOARD_STEP_PIN_Z;
        case 3:
            return BOARD_STEP_PIN_A;
        default:
            return 0;
    }
}

static uint16_t axis_dir_pin(uint8_t axis) {
    switch (axis) {
        case 0:
            return BOARD_DIR_PIN_X;
        case 1:
            return BOARD_DIR_PIN_Y;
        case 2:
            return BOARD_DIR_PIN_Z;
        case 3:
            return BOARD_DIR_PIN_A;
        default:
            return 0;
    }
}

void drv_gpio_init_safe(void) {
    board_init_gpio();
}

void drv_gpio_stepper_enable(bool enable) {
#if BOARD_DRV_EN_ACTIVE_LOW
    if (enable) {
        gpio_clear(BOARD_DRV_EN_PORT, BOARD_DRV_EN_PIN);
    } else {
        gpio_set(BOARD_DRV_EN_PORT, BOARD_DRV_EN_PIN);
    }
#else
    if (enable) {
        gpio_set(BOARD_DRV_EN_PORT, BOARD_DRV_EN_PIN);
    } else {
        gpio_clear(BOARD_DRV_EN_PORT, BOARD_DRV_EN_PIN);
    }
#endif
}

void drv_gpio_set_dir(uint8_t axis, bool dir_positive) {
    const uint16_t pin = axis_dir_pin(axis);
    if (pin == 0) {
        return;
    }

    if (dir_positive) {
        gpio_set(BOARD_DIR_PORT, pin);
    } else {
        gpio_clear(BOARD_DIR_PORT, pin);
    }
}

void drv_gpio_emit_step_mask(uint32_t axis_mask) {
    uint16_t pins = 0;
    for (uint8_t axis = 0; axis < BOARD_AXIS_COUNT; axis++) {
        if ((axis_mask & (1u << axis)) != 0u) {
            pins |= axis_step_pin(axis);
            g_pending_step_mask |= (1u << axis);
        }
    }

    if (pins != 0) {
        gpio_set(BOARD_STEP_PORT, pins);
    }
}

void drv_gpio_clear_step_mask(uint32_t axis_mask) {
    uint16_t pins = 0;
    for (uint8_t axis = 0; axis < BOARD_AXIS_COUNT; axis++) {
        if ((axis_mask & (1u << axis)) != 0u) {
            pins |= axis_step_pin(axis);
            g_pending_step_mask &= ~(1u << axis);
        }
    }

    if (pins != 0) {
        gpio_clear(BOARD_STEP_PORT, pins);
    }
}

void drv_gpio_clear_pending_steps(void) {
    const uint32_t mask = g_pending_step_mask;
    if (mask == 0) {
        return;
    }

    drv_gpio_clear_step_mask(mask);
}

uint32_t drv_gpio_read_inputs_raw(void) {
    uint32_t bits = 0;

    if (gpio_get(BOARD_INPUT_PORT, BOARD_INPUT_LIMIT_X_PIN) == 0) {
        bits |= BOARD_INPUT_BIT_LIMIT_X;
    }
    if (gpio_get(BOARD_INPUT_PORT, BOARD_INPUT_LIMIT_Y_PIN) == 0) {
        bits |= BOARD_INPUT_BIT_LIMIT_Y;
    }
    if (gpio_get(BOARD_INPUT_PORT, BOARD_INPUT_LIMIT_Z_PIN) == 0) {
        bits |= BOARD_INPUT_BIT_LIMIT_Z;
    }
    if (gpio_get(BOARD_INPUT_PORT, BOARD_INPUT_ESTOP_PIN) == 0) {
        bits |= BOARD_INPUT_BIT_ESTOP;
    }
    if (gpio_get(BOARD_INPUT_PORT, BOARD_INPUT_HAND_PIN) == 0) {
        bits |= BOARD_INPUT_BIT_HAND;
    }

    return bits;
}
