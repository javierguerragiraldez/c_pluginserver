#pragma once

#include <stddef.h>

typedef struct {
	char *name;
    int (*rewrite)(int);
    int (*access)(int);
    int (*header_filter)(int);
    int (*body_filter)(int);
    int (*log)(int);
} plugin_t;

const plugin_t *get_plugin(const char *name);
