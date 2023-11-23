#ifndef CUTTERENG_ASSERT_H
#define CUTTERENG_ASSERT_H

#include "log.h"

#ifdef DEBUG
#define ASSERT(expr) \
	do { \
		if(!(expr)) { \
			LOG_ERROR("Assertion failed: "#expr); \
			exit(1); \
		} \
	} while(0)
#else
#define ASSERT(expr)
#endif // DEBUG

#endif // CUTTERENG_ASSERT_H
