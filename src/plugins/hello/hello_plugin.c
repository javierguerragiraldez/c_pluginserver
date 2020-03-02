#include "plugins/plugin.h"

int hello_access(int evt_id);

plugin_t hello_reg = {
	"c-hello",
	NULL,
	&hello_access,
	NULL,
	NULL,
	NULL
};

int hello_access(int evt_id)
{
	(void)evt_id;
	return -1;
}
