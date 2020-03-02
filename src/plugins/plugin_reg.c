#include <string.h>
#include "plugin.h"

static plugin_t reg = {
	hello_reg,
	{ NULL, },
};

const plugin_t *get_plugin(const char *name) {
	for (plugin_t *p = &reg; p->name != NULL; p++) {
		if (strcmp(name, p->name) == 0)
			return p;
	}
}
