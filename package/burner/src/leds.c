//#define DEBUG
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include "leds.h"
#include "common.h"

int led_prop_open(const char *led, const char *prop)
{
	char path[256];
	int len;

	len = snprintf(path, sizeof(path), "/sys/class/leds/%s/%s", led, prop);
	if (len < 0 || len >= sizeof(path))
		return -1;

	return open(path, O_WRONLY | O_CLOEXEC);
}

int leds_prop_open(const char *leds[], size_t leds_len, const char *prop,
		   int fds[], size_t fds_len)
{
	/* Make it safe to call fds_close first */
	memset(fds, -1, sizeof(int) * fds_len);

	if (fds_len < leds_len)
		return -1;

	for (int i = 0; i < leds_len; i++) {
		const char *led = leds[i];
		int fd;

		fd = led_prop_open(led, prop);
		if (fd == -1)
			return -1;

		fds[i] = fd;
	}

	return 0;
}

int led_set(const char *led, const char *prop, const char *val)
{
	int fd;
	int ret;

	fd = led_prop_open(led, prop);
	if (fd == -1)
		return -1;

	if (write(fd, val, strlen(val)) == -1)
		ret = -1;
	else
		ret = 0;

	close(fd);

	return ret;
}

int leds_set(const char *leds[], size_t leds_len, const char *prop, const char *val)
{
	int fds[MAX_LEDS];
	int ret;
	size_t val_len;

	ret = leds_prop_open(leds, leds_len, prop, fds, ARRAY_SIZE(fds));
	if (ret == -1)
		goto out;

	val_len = strlen(val);

	for (int i = 0; i < leds_len; i++) {
		debug("%s/%s = \"%s\"\n", leds[i], prop, val);
		if (write(fds[i], val, val_len) == -1) {
			ret = -1;
			break;
		}
	}

out:
	fds_close(fds, ARRAY_SIZE(fds));

	return ret;
}
