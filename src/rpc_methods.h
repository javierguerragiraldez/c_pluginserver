#pragma once

#define DILL_DISABLE_RAW_NAMES
#include <libdill.h>

#include "errtrace.h"
#include "cwpack/cwpack.h"


int set_plugindir(cw_unpack_context *in, cw_pack_context *out);
int get_plugininfo(cw_unpack_context *in, cw_pack_context *out);
int start_instance(cw_unpack_context *in, cw_pack_context *out);
int handle_event(cw_unpack_context *in, cw_pack_context *out);
