#include "common.h"

void ReleaseAssertFailure(const char *__assertion, const char *__file,
			   unsigned int __line, const char *__function)
{
	printf("%s:%u: %s: Assertion `%s' failed.\n", __file, __line, __function, __assertion);
	exit(0);
}

