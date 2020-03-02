#pragma once

#ifdef DEBUG
#include <string.h>
#include <stdio.h>
#endif

#define none ((void)0)

#define T()	fprintf(stderr, "[%s:%d] %s: %s\n", __FILE__, __LINE__, __func__, strerror(errno))
#define Tv(v) ({T(); (v);})
#define Tf(l) ({T(); goto l;})
#define Tcont() ({T(); continue;})
