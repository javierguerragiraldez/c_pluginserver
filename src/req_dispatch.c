
#define DILL_DISABLE_RAW_NAMES
#include <libdill.h>

#include "errtrace.h"
#include "cwpack/cwpack.h"
#include "req_dispatch.h"
#include "rpc_methods.h"

typedef int rpc_handler(cw_unpack_context *in, cw_pack_context *out);
typedef struct {
	const char *name;
	rpc_handler *handler;
} handler_reg;



static handler_reg rpc_methods[] = {
	{ "plugin.SetPluginDir", set_plugindir },
	{ NULL, NULL },
};

static int dispatch_method(
	uint64_t msgid,
	const char *mtdh,
	cw_unpack_context *in,
	cw_pack_context *out
) {
	for (handler_reg *h = rpc_methods; h->name != NULL; h++) {
		if (strcmp(mtdh, h->name) == 0) {
			cw_pack_array_size(out, 4);
			cw_pack_unsigned(out, 1);
			cw_pack_unsigned(out, msgid);
			return h->handler(in, out);
		}
	}
	fprintf(stderr, "method %s not found\n", mtdh);
	return Tv(-1);
}


int dispatch_request(cw_unpack_context *ctx_in, cw_pack_context *ctx_out) {
	cw_unpack_next(ctx_in);
	if (errno != 0) return Tv(-1);

	if (ctx_in->item.type != CWP_ITEM_ARRAY || ctx_in->item.as.array.size != 4) {
		fprintf(stderr, "expected 4-item array\n");
		return Tv(-1);
	}

	cw_unpack_next(ctx_in);
	if (errno != 0) return Tv(-1);

	if (ctx_in->item.type != CWP_ITEM_POSITIVE_INTEGER || ctx_in->item.as.u64 != 0) {
		fprintf(stderr, "expected proc request, type 0\n");
		return Tv(-1);
	}

	cw_unpack_next(ctx_in);
	if (errno != 0) return Tv(-1);

	if (ctx_in->item.type != CWP_ITEM_POSITIVE_INTEGER) {
		fprintf(stderr, "expected positive integer 'msgid'\n");
		return Tv(-1);
	}

	uint64_t msgid = ctx_in->item.as.u64;

	cw_unpack_next(ctx_in);
	if (errno != 0) return Tv(-1);

	if (ctx_in->item.type != CWP_ITEM_STR) {
		fprintf(stderr, "expected string 'method name'");
		return Tv(-1);
	}

	const char *mthd = strndup(ctx_in->item.as.str.start, ctx_in->item.as.str.length);
	if (!mthd) return Tv(-1);

	int rc = dispatch_method(msgid, mthd, ctx_in, ctx_out);

	free((void *)mthd);
	return rc;
}

