#pragma once

#include <stdio.h>
#include <unistd.h>

#define __maybe_unused	__attribute__((unused))
#define ARRAY_SIZE(x)	(sizeof(x) / sizeof(*x))

__maybe_unused
static void fds_close(int fds[], size_t fds_len)
{
	for (int i = 0; i < fds_len; i++) {
		if (fds[i] != -1)
			close(fds[i]);
	}
}

/* Dummy printf to keep format checking */
#define no_printf(fmt, ...)			\
({						\
	if (0)					\
		printf(fmt, ##__VA_ARGS__);	\
	0;					\
})

#ifdef DEBUG
#define debug(fmt, ...) \
	printf(fmt, ##__VA_ARGS__)
#else
#define debug(fmt, ...) \
	no_printf(fmt, ##__VA_ARGS__)
#endif
