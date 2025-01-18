#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>

#include "structs.h"


#ifndef DIE
#define DIE
extern void
die(const char *fmt, ...);

extern int
dumps_yeet_arr(struct yeet **yeets);
#endif