/*
 * How stupid it does not sound, but there is no way from a shell script to
 * know if a button is pressed at any given moment. E.g. there is no way to get
 * the state of GPIO of a button that is used as an input device. And there is
 * no way to get the state somewhere from sysfs. The only way is to use ioctl
 * which is obviously cannot be easily done from a shell script.
 */
#include <errno.h>
#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "common.h"

#define INPUT_DEV	"/dev/input/event1"

int main(void)
{
	int i;
	int val;
	int ret;
	int fd_input;
	struct libevdev *dev;
	unsigned int codes[] = { BTN_0, BTN_1, BTN_2 };

	fd_input = open(INPUT_DEV, O_RDONLY | O_NONBLOCK);
	if (fd_input == -1) {
		fprintf(stderr, "Cannot open " INPUT_DEV ": %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	ret = libevdev_new_from_fd(fd_input, &dev);
	if (ret < 0) {
		fprintf(stderr, "Failed to init libevdev: %s\n", strerror(-ret));
		exit(EXIT_FAILURE);
	}

	for (i = 0; i < ARRAY_SIZE(codes); i++) {
		if (!libevdev_has_event_code(dev, EV_KEY, codes[i])) {
			fprintf(stderr, "Error: input code %s is not available on " INPUT_DEV "\n",
				libevdev_event_code_get_name(EV_KEY, codes[i]));
			exit(EXIT_FAILURE);
		}
	}

	for (i = 0; i < ARRAY_SIZE(codes); i++) {
		if (libevdev_fetch_event_value(dev, EV_KEY, codes[i], &val) && val)
			return 0;
	}

	return 1;
}
