#include "drv_uart.h"

#include "../board/board_init.h"
#include "../board/board_pins.h"

#include <libopencm3/cm3/nvic.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/usart.h>

#define UART_RX_BUFFER_SIZE 256U

static volatile uint8_t g_rx_buffer[UART_RX_BUFFER_SIZE];
static volatile uint16_t g_rx_head = 0;
static volatile uint16_t g_rx_tail = 0;

static void rx_push(uint8_t ch) {
    const uint16_t next = (uint16_t)((g_rx_head + 1U) % UART_RX_BUFFER_SIZE);
    if (next == g_rx_tail) {
        return;
    }

    g_rx_buffer[g_rx_head] = ch;
    g_rx_head = next;
}

void usart2_isr(void) {
    if ((usart_get_flag(BOARD_USART, USART_ISR_RXNE_RXFNE) != 0) &&
        (usart_get_interrupt_source(BOARD_USART, USART_CR1_RXNEIE_RXFNEIE) != 0)) {
        rx_push((uint8_t)usart_recv(BOARD_USART));
    }
}

void drv_uart_init(void) {
    rcc_periph_clock_enable(BOARD_USART_RCC);
    board_init_uart_pins();

    usart_set_baudrate(BOARD_USART, 115200);
    usart_set_databits(BOARD_USART, 8);
    usart_set_stopbits(BOARD_USART, USART_STOPBITS_1);
    usart_set_mode(BOARD_USART, USART_MODE_TX_RX);
    usart_set_parity(BOARD_USART, USART_PARITY_NONE);
    usart_set_flow_control(BOARD_USART, USART_FLOWCONTROL_NONE);

    usart_enable_rx_interrupt(BOARD_USART);
    nvic_enable_irq(NVIC_USART2_IRQ);
    usart_enable(BOARD_USART);
}

size_t drv_uart_read(uint8_t *dst, size_t cap) {
    size_t read = 0;

    while ((read < cap) && (g_rx_tail != g_rx_head)) {
        dst[read++] = g_rx_buffer[g_rx_tail];
        g_rx_tail = (uint16_t)((g_rx_tail + 1U) % UART_RX_BUFFER_SIZE);
    }

    return read;
}

size_t drv_uart_write(const uint8_t *src, size_t len) {
    size_t written = 0;

    while (written < len) {
        usart_send_blocking(BOARD_USART, src[written]);
        written++;
    }

    return written;
}
