
#define DILL_DISABLE_RAW_NAMES
#include <libdill.h>

#include "errtrace.h"
#include "cwpack/cwpack.h"
#include "mpack/tools.h"
#include "plugins/plugin.h"
#include "req_dispatch.h"

#define mp_get_string(ctx)	({	\
	cw_unpack_next(ctx);	\
	((ctx->item.type == CWP_ITEM_STR) ? ctx->item.as.str.start : Tv(NULL)); \
})

#define mp_put_const(ctx, v)	_Generic((v), \
	char: cw_pack_str((ctx), (v), sizeof(v)),	\
	default: T()	\
)

#define mp_put_okorerror(ctx, err) ({	\
	if (err != 0) {	\
		mp_put_err(out, err);	\
		cw_pack_nil(out);	\
	} else {	\
		cw_pack_nil(out);	\
		mp_put_const(out, "Ok");	\
	}	\
	(err);	\
})

// TODO: check about strerrorlen_s
#define mp_put_err(ctx, err)	cw_pack_str(ctx, strerror(err), strlen(strerror(err)))

static int set_plugindir(cw_unpack_context *in, cw_pack_context *out) {
	const char *dir = mp_get_string(in);
	if (!dir) Tf(end);

	if (plugin_set_dir(dir) < 0) Tf(end);

end:
	return mp_put_okorerror(out, errno);
}


static int get_plugininfo(cw_unpack_context *in, cw_pack_context *out) {
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
	cw_pack_map_size(out, 0);

	return 0;
}


static int start_instance(cw_unpack_context *in, cw_pack_context *out) {
	cw_skip_items(in, 6);		// [{"Config":"xxx", "Name":"c-hello"}]

	cw_pack_nil(out);	// no error;
	cw_pack_map_size(out, 1);
	cw_pack_str(out, "Id", 2);
	cw_pack_unsigned(out, 1);

	return 0;
}


static int handle_event(cw_unpack_context *in, cw_pack_context *out){
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


static struct {
	const char *name;
	int (*handler)(cw_unpack_context *in, cw_pack_context *out);
} rpc_methods[] = {
	{ "plugin.SetPluginDir", set_plugindir },
	{ "plugin.GetPluginInfo", get_plugininfo },
	{ "plugin.StartInstance", start_instance },
	{ "plugin.HandleEvent", handle_event },
	{ NULL, NULL },
};


static int dispatch_method(
	uint64_t msgid,
	const char *mtdh,
	cw_unpack_context *in,
	cw_pack_context *out
) {
	for (typeof(rpc_methods[0]) *h = rpc_methods; h->name != NULL; h++) {
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

