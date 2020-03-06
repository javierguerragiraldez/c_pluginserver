
#include "plugin.h"

int plugin_set_dir(const char *name) {
	(void) name;
	return 0;
}

int plugin_get(const char *name, plugin_t *p) {
	(void) name;
	(void) p;
	return 0;
}

int plugin_start_instance(const char *name, const char *config) {
	(void) name;
	(void) config;

	return 1;
}

int plugin_handle_event(int inst_id, const char *evt_name) {
	(void) inst_id;
	(void) evt_name;

	return 0;
}

