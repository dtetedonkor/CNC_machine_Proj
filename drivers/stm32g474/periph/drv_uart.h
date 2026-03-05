#pragma once

#include <stddef.h>
#include <stdint.h>

void drv_uart_init(void);
size_t drv_uart_read(uint8_t *dst, size_t cap);
size_t drv_uart_write(const uint8_t *src, size_t len);
