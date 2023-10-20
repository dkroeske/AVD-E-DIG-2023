/**
 * Copyright (c) 2020 Raspberry Pi (Trading) Ltd.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

#ifndef WS2812_INC
#define WS2812_INC

#define NUM_PIXELS 1
#define WS2812_PIN 28 // WS2811 on Cytron board

void pixel(uint8_t r, uint8_t g, uint8_t b);
void pattern_sparkle(uint len, uint t);
void ws2812_init();


#endif
