#include "rpc_methods.h"

int handle_event(cw_unpack_context *in, cw_pack_context *out){
	cw_unpack_next(in);
	if (errno != 0) return Tv(-1);
	if (in->item.type != CWP_ITEM_ARRAY || in->item.as.array.size != 1)
		return Tv(-1);

	cw_unpack_next(in);
	if (errno != 0) return Tv(-1);
	if (in->item.type != CWP_ITEM_MAP)
		return Tv(-1);

	int n_items = in->item.as.map.size;

	while (n_items > 0) {
		cw_unpack_next(in);
		if (errno != 0) return Tv(-1);
		if (in->item.type != CWP_ITEM_STR)
			return Tv(-1);

		const cwpack_blob *str = &in->item.as.str;
		if (str->length == 9 && strncmp(str->start, "EventName", 9) == 0) {


		} else if (str->length == 10 && strncmp(str->start, "InstanceId", 10) == 0) {

		} else if (str->length == 6 && strncmp(str->start, "Params", 6) == 0) {

		}

		n_items--;
	}

	cw_pack_nil(out);
	cw_pack_map_size(out, 2);
	cw_pack_str(out, "EventId", 7);
	cw_pack_unsigned(out, 1);
	cw_pack_str(out, "Data", 4);
	cw_pack_map_size(out, 2);
	cw_pack_str(out, "Method", 6);
	cw_pack_str(out, "kong.request.set_header", 23);
	cw_pack_str(out, "Args", 4);
	cw_pack_array_size(out, 2);
	cw_pack_str(out, "x-hello", 7);
	cw_pack_str(out, "hello from C", 12);
	cw_pack_unsigned(out, 1000);

	return 0;
}

