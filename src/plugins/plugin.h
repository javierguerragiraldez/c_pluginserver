#pragma once

typedef struct {
	char *name;
    int (*rewrite)(int);
    int (*access)(int);
    int (*header_filter)(int);
    int (*body_filter)(int);
    int (*log)(int);
} plugin_t;

int plugin_set_dir(const char *name);
int plugin_get(const char *name, plugin_t *p);
