#include "rpc_methods.h"

int start_instance(cw_unpack_context *in, cw_pack_context *out) {
	cw_skip_items(in, 6);		// [{"Config":"xxx", "Name":"c-hello"}]

	cw_pack_nil(out);	// no error;
	cw_pack_map_size(out, 1);
	cw_pack_str(out, "Id", 2);
	cw_pack_unsigned(out, 1);

	return 0;
}
