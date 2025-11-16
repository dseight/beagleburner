//#define DEBUG
#include <errno.h>
#include <fcntl.h>
#include <libevdev/libevdev.h>
#include <poll.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdckdint.h>
#include <string.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include "common.h"
#include "leds.h"

#define INPUT_DEV	"/dev/input/event1"
#define SDCARD_DEV_NAME	"mmcblk0"

#define LED_SLOT1	"yellow:status-1"
#define LED_SLOT2	"yellow:status-2"
#define LED_SLOT3	"yellow:status-3"
#define LED_OK		"green:status-4"
#define LED_FAULT	"red:fault"

static const char *all_leds[] = {
	LED_SLOT1,
	LED_SLOT2,
	LED_SLOT3,
	LED_OK,
	LED_FAULT,
};

static const char *slot_leds[] = {
	LED_SLOT1,
	LED_SLOT2,
	LED_SLOT3,
};

static struct libevdev *dev;
static int fd_inotify;

static void fail(void)
{
	leds_set(all_leds, ARRAY_SIZE(all_leds), "trigger", "none");
	led_set(LED_FAULT, "trigger", "default-on");
	exit(EXIT_FAILURE);
}

static bool timespec_subtract(struct timespec *r,
			      struct timespec x, struct timespec y)
{
	/* Compute nanoseconds, setting borrow to 1 or 0 for propagation into seconds. */
	long int nsec_diff = x.tv_nsec - y.tv_nsec;
	int borrow = nsec_diff < 0;
	r->tv_nsec = nsec_diff + 1000000000 * borrow;

	/* Compute seconds, returning true if this overflows. */
	bool v = ckd_sub(&r->tv_sec, x.tv_sec, y.tv_sec);
	return v ^ ckd_sub(&r->tv_sec, r->tv_sec, borrow);
}

static uint32_t get_last_event_mask(int fd, const char *fname)
{
	char buf[4096] __attribute__ ((aligned(__alignof__(struct inotify_event))));
	const struct inotify_event *event;
	ssize_t len;
	uint32_t last_mask = 0;

	while (1) {
		len = read(fd, buf, sizeof(buf));
		if (len == -1 && errno != EAGAIN) {
			perror("read");
			fail();
		}

		/* No events left to read */
		if (len <= 0)
			break;

		for (char *ptr = buf; ptr < buf + len;
			ptr += sizeof(struct inotify_event) + event->len) {
			event = (const struct inotify_event *) ptr;

			if (event->len && !strcmp(event->name, fname))
				last_mask = event->mask;
		}
	}

	return last_mask;
}

static int wait_for_inotify_event(int fd, const char *fname, uint32_t mask)
{
	int poll_num;
	nfds_t nfds = 1;
	struct pollfd pfd = {
		.fd = fd,
		.events = POLLIN,
	};

	while (1) {
		poll_num = poll(&pfd, nfds, -1);
		if (poll_num == -1) {
			if (errno == EINTR)
				continue;
			perror("poll");
			fail();
		}

		if (poll_num == 0)
			continue;

		if (pfd.revents & POLLIN) {
			if (get_last_event_mask(fd, fname) & mask)
				break;
		}
	}

	return 0;
}

static int input_code_to_slot(uint16_t code)
{
	switch (code) {
	case BTN_0:
		return 1;
	case BTN_1:
		return 2;
	case BTN_2:
		return 3;
	}
	return -1;
}

static void flush_events(void)
{
	struct input_event ev;

	while (libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev) == 0)
		;;
}

/*
 * Returns negative code on error, and slot number (starting from 1) otherwise.
 */
static int wait_for_slot_selection(void)
{
	int fds[MAX_LEDS];
	int ret;
	struct timespec ts, ts_now, ts_diff;
	struct input_event ev;

	/*
	 * Kernel's "timer" trigger is an inconsistent drifting piece of shit,
	 * which can't be used for blinking LEDs synchronously.
	 */
	leds_set(slot_leds, ARRAY_SIZE(slot_leds), "trigger", "oneshot");

	ret = leds_prop_open(slot_leds, ARRAY_SIZE(slot_leds), "shot", fds, ARRAY_SIZE(fds));
	if (ret)
		return ret;

	/* When changing delays, also adjust usleep value below */
	/* 3 Hz 50% duty cycle */
	leds_set(slot_leds, ARRAY_SIZE(slot_leds), "delay_off", "333");
	leds_set(slot_leds, ARRAY_SIZE(slot_leds), "delay_on", "333");

	ts.tv_sec = 0;
	ts.tv_nsec = 0;

	while (1) {
		clock_gettime(CLOCK_MONOTONIC, &ts_now);
		timespec_subtract(&ts_diff, ts_now, ts);

		if (ts_diff.tv_nsec >= 666000000L || ts_diff.tv_sec >= 1) {
			debug("ts_diff.tv_sec = %lld, ts_diff.tv_nsec = %ld\n",
			      ts_diff.tv_sec, ts_diff.tv_nsec);

			ts = ts_now;

			for (int i = 0; i < ARRAY_SIZE(slot_leds); i++) {
				debug("%s/shot = \"1\"\n", slot_leds[i]);
				if (write(fds[i], "1", 1) == -1) {
					ret = -1;
					break;
				}
			}
		}

		ret = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_NORMAL, &ev);
		if (ret == 0 && ev.type == EV_KEY) {
			int slot = input_code_to_slot(ev.code);

			if (slot < 0) {
				printf("Unknown event: %s %s %d\n",
					libevdev_event_type_get_name(ev.type),
					libevdev_event_code_get_name(ev.type, ev.code),
					ev.value);
			} else {
				ret = slot;
				break;
			}
		}

		if (get_last_event_mask(fd_inotify, SDCARD_DEV_NAME) & IN_DELETE) {
			ret = -1;
			break;
		}

		usleep(1000);
	}

	fds_close(fds, ARRAY_SIZE(fds));

	return ret;
}

static int run_slot(int slot)
{
	char slot_str[16];
	int len;
	pid_t pid, w;
	int wstatus;

	len = snprintf(slot_str, sizeof(slot_str), "%d", slot);
	if (len < 0 || len >= sizeof(slot_str))
		return EXIT_FAILURE;

	pid = fork();
	if (pid == -1) {
		perror("fork");
		return EXIT_FAILURE;
	}

	char *argv[] = { "/usr/bin/burner-slot-handler", slot_str, NULL };

	if (pid == 0) {
		int ret = execv(argv[0], argv);
		if (ret == -1) {
			perror("exec");
			exit(EXIT_FAILURE);
		} else {
			exit(ret);
		}
	} else {
		w = wait(&wstatus);
		if (w == -1) {
			perror("wait");
			return EXIT_FAILURE;
		}
		if (WIFEXITED(wstatus))
			return WEXITSTATUS(wstatus);
		if (WIFSIGNALED(wstatus)) {
			fprintf(stderr, "%s was terminated by a signal %d\n",
				argv[0], WTERMSIG(wstatus));
			return EXIT_FAILURE;
		}
		/* Something else? */
		return EXIT_FAILURE;
	}
}

/*
 * Sequence:
 *  1. turn off leds
 *  2. register inotify
 *  3. check sdcard presence
 *  4. wait for sdcard if needed
 *  5. when sdcard is present, start blinking yellow leds
 *  6. wait for slot selection from libevdev
 *  7. when slot selected, start to blink only 1 yellow led and quicker
 *  8. start programming
 *  9. when programming finished, make both yellow and green to glow continuously
 * 10. if any error happened, light up red led
 * 11. when sdcard is pulled off, turn off leds
 * 12. goto 3
 */
int main(void)
{
	int fd_input;
	int wd;
	int ret;
	struct stat sb;

	/* Turn off LEDs to indicate that the program has started */
	leds_set(all_leds, ARRAY_SIZE(all_leds), "trigger", "none");

	fd_inotify = inotify_init1(IN_NONBLOCK);
	if (fd_inotify == -1) {
		perror("inotify_init1");
		fail();
	}

	/* TODO: glow up one of bbb leds to show sd card presence. Maybe in a separate process. */
	wd = inotify_add_watch(fd_inotify, "/dev", IN_CREATE | IN_DELETE);
	if (wd == -1) {
		fprintf(stderr, "Cannot watch /dev: %s\n", strerror(errno));
		fail();
	}

	fd_input = open(INPUT_DEV, O_RDONLY | O_NONBLOCK);
	if (fd_input == -1) {
		fprintf(stderr, "Cannot open " INPUT_DEV ": %s\n", strerror(errno));
		fail();
	}

	ret = libevdev_new_from_fd(fd_input, &dev);
	if (ret < 0) {
		fprintf(stderr, "Failed to init libevdev: %s\n", strerror(-ret));
		fail();
	}

	if (!libevdev_has_event_code(dev, EV_KEY, BTN_0) ||
	    !libevdev_has_event_code(dev, EV_KEY, BTN_1) ||
	    !libevdev_has_event_code(dev, EV_KEY, BTN_2)) {
		fprintf(stderr, "Error: one of the input codes (BTN_0, BTN_1 or BTN_2)"
			"is not available on " INPUT_DEV "\n");
		fail();
	}

	while (1) {
		int slot;

		/*
		 * Turn off LEDs once again. This is needed in case we are on the next
		 * loop iteration.
		 */
		leds_set(all_leds, ARRAY_SIZE(all_leds), "trigger", "none");

		ret = stat("/dev/" SDCARD_DEV_NAME, &sb);
		if (ret == -1 && errno != ENOENT) {
			perror("stat");
			fail();
		}

		if (ret == -1)
			wait_for_inotify_event(fd_inotify, SDCARD_DEV_NAME, IN_CREATE);

		/* Some events might have been left from previous iteration. Drop them. */
		flush_events();

		slot = wait_for_slot_selection();
		/* sdcard was probably removed during slot selection */
		if (slot == -1)
			continue;

		/* Start to quickly blink with slot LED. */
		leds_set(all_leds, ARRAY_SIZE(all_leds), "trigger", "none");
		led_set(slot_leds[slot - 1], "trigger", "timer");
		led_set(slot_leds[slot - 1], "delay_off", "160");
		led_set(slot_leds[slot - 1], "delay_on", "160");

		if (run_slot(slot) != 0)
			fail();

		leds_set(all_leds, ARRAY_SIZE(all_leds), "trigger", "none");
		led_set(slot_leds[slot - 1], "trigger", "default-on");
		led_set(LED_OK, "trigger", "default-on");

		wait_for_inotify_event(fd_inotify, SDCARD_DEV_NAME, IN_DELETE);
	}

        return 0;
}
