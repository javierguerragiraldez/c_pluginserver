
#include "rpc_methods.h"

int get_plugininfo(cw_unpack_context *in, cw_pack_context *out) {
	cw_unpack_next(in);
	if (errno != 0) return Tv(-1);

	if (in->item.type != CWP_ITEM_ARRAY || in->item.as.array.size != 1)
		return Tv(-1);

	cw_unpack_next(in);
	if (errno != 0) return Tv(-1);

	if (in->item.type != CWP_ITEM_STR)
		return Tv(-1);

	fprintf(stderr, "info for plugin %*s\n",
			in->item.as.str.length, (const char *)in->item.as.str.start);

	cw_pack_nil(out);	// no error
// 	cw_pack_str(out, "some noise", 10);
	cw_pack_map_size(out, 0);

	return 0;
}
