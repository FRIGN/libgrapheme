/* See LICENSE file for copyright and license details. */
#include <time.h>

#include "util.h"

double
time_diff(struct timespec *a, struct timespec *b)
{
	return (double)(b->tv_sec - a->tv_sec) +
	       (double)(b->tv_nsec - a->tv_nsec) * 1E-9;
}
