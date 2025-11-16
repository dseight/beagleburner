#pragma once

#include <stddef.h>

#define MAX_LEDS	8

int led_prop_open(const char *led, const char *prop);
int leds_prop_open(const char *leds[], size_t leds_len, const char *prop,
		   int fds[], size_t fds_len);
int led_set(const char *led, const char *prop, const char *val);
int leds_set(const char *leds[], size_t leds_len, const char *prop, const char *val);
