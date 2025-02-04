#pragma once

#include "stdio.h"

#define DO(cmd, ...)			\
	if (cmd(__VA_ARGS__) == -1)	\
	{							\
		perror("bad "#cmd);		\
		return 0;				\
	}

#ifdef ENABLE_LOGGING

#define LOG(buf, ...) \
	printf(buf, __VA_ARGS__);

#define MSG(buf) \
	printf(buf);

#else

#define LOG(buf, ...)
#define MSG(buf)

#endif
