#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>

/* STM32G491RE + 4x TMC2209 STEP/DIR/EN "hello world" pulse demo */
#define STEP_PINS_MASK (GPIO0 | GPIO1 | GPIO2 | GPIO3)
#define DIR_PINS_MASK  (GPIO4 | GPIO5 | GPIO6 | GPIO7)
#define EN_PIN         GPIO0
/* Cycle delays below assume reset/default clock and are for quick bench testing only. */
#define PULSE_COUNT_PER_BURST 200U
#define STEP_PULSE_DELAY_CYCLES 4000U
#define DIR_CHANGE_DELAY_CYCLES 1200000U

static void delay_cycles(volatile uint32_t cycles) {
    while (cycles--) {
        __asm__("nop");
    }
}

static void gpio_setup(void) {
    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);

    gpio_mode_setup(GPIOA, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, STEP_PINS_MASK | DIR_PINS_MASK);
    gpio_mode_setup(GPIOB, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, EN_PIN);

    /* TMC2209 EN is active-low */
    gpio_clear(GPIOB, EN_PIN);
}

int main(void) {
    /* Keep default reset clock; this is a basic STEP pulse smoke test, not precise timing. */
    gpio_setup();
    gpio_set(GPIOA, DIR_PINS_MASK);

    while (1) {
        for (uint32_t i = 0; i < PULSE_COUNT_PER_BURST; i++) {
            gpio_set(GPIOA, STEP_PINS_MASK);
            delay_cycles(STEP_PULSE_DELAY_CYCLES);
            gpio_clear(GPIOA, STEP_PINS_MASK);
            delay_cycles(STEP_PULSE_DELAY_CYCLES);
        }

        delay_cycles(DIR_CHANGE_DELAY_CYCLES);
        gpio_toggle(GPIOA, DIR_PINS_MASK);
        delay_cycles(DIR_CHANGE_DELAY_CYCLES);
    }
}
