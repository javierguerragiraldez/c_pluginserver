
#define DILL_DISABLE_RAW_NAMES
#include <libdill.h>

#include "errtrace.h"
#include "cwpack/cwpack.h"
#include "mpack/tools.h"
#include "plugins/plugin.h"
#include "req_dispatch.h"

#define mp_get_uint(ctx)	({	\
	cw_unpack_next(ctx);	\
	(((ctx)->item.type == CWP_ITEM_POSITIVE_INTEGER) ? (ctx)->item.as.u64 : Tv((uint64_t)-1));	\
})

#define mp_get_string(ctx)	({	\
	cw_unpack_next(ctx);	\
	(((ctx)->item.type == CWP_ITEM_STR) ? (ctx)->item.as.str.start : Tv(NULL)); \
})

#define mp_string_dict_start(ctx) do {	\
	cw_unpack_next(ctx);	\
	if ((ctx)->item.type != CWP_ITEM_MAP) Tf(_mp_dict_fail);	\
	for(int n = (ctx)->item.as.map.size; n > 0; n--) {	\
		cw_unpack_next(ctx);	\
		if ((ctx)->item.type != CWP_ITEM_STR) Tf(_mp_dict_fail);	\
		const cwpack_blob str = (ctx)->item.as.str;

#define mp_string_dict_case(key, action)	\
		if (str.length == sizeof(key) && strncmp(str.start, (key), sizeof(key)))	\
			action;	\
		else

#define mp_string_dict_end(fail)	\
		{;}	\
	}	\
	break;	\
	_mp_dict_fail:	\
	fail;	\
} while(0)

#define mp_put_lit(ctx, s) cw_pack_str((ctx), (s), sizeof(s))
#define mp_put_str(ctx, s) cw_pack_str((ctx), (s), strlen(s))

// TODO: check about strerrorlen_s
#define mp_put_err(ctx, err)	mp_put_str(ctx, strerror(err))

#define mp_put_okorerror(ctx, err) ({	\
	if (err != 0) {	\
		mp_put_err(out, err);	\
		cw_pack_nil(out);	\
	} else {	\
		cw_pack_nil(out);	\
		mp_put_lit(out, "Ok");	\
	}	\
	(err);	\
})

#define mp_expect_arraysz(ctx, sz, fail) ({ \
	cw_unpack_next(ctx);	\
	if ((ctx)->item.type != CWP_ITEM_ARRAY || \
		(ctx)->item.as.array.size != (sz) \
	) { fail; } \
})


static int set_plugindir(cw_unpack_context *in, cw_pack_context *out) {
	const char *dir = mp_get_string(in);
	if (!dir) Tf(end);

	if (plugin_set_dir(dir) < 0) Tf(end);

end:
	return mp_put_okorerror(out, errno);
}


static int get_plugininfo(cw_unpack_context *in, cw_pack_context *out) {
	mp_expect_arraysz(in, 1, return Tv(-1));
	const char *name = mp_get_string(in);
	if (!name) return Tv(-1);

	fprintf(stderr, "info for plugin %*s\n",
			in->item.as.str.length, name);

	cw_pack_nil(out);	// no error
	cw_pack_map_size(out, 0);	// TODO: send some info

	return 0;
}

static int start_instance(cw_unpack_context *in, cw_pack_context *out) {
	mp_expect_arraysz(in, 1, return Tv(-1));

	const char *config;
	const char *name;

	mp_string_dict_start(in)
		mp_string_dict_case("Name", name = mp_get_string(in))
		mp_string_dict_case("Config", config = mp_get_string(in))
	mp_string_dict_end(return Tv(-1));

	int id = plugin_start_instance(name, config);
	if (id < 0) Tf(error);

	cw_pack_nil(out);	// no error;
	cw_pack_map_size(out, 1);
	mp_put_lit(out, "Id");
	cw_pack_unsigned(out, id);

	return 0;

error:
	mp_put_err(out, errno);
	cw_pack_nil(out);
	return Tv(-1);
}


static int handle_event(cw_unpack_context *in, cw_pack_context *out) {
	mp_expect_arraysz(in, 1, return Tv(-1));

	const char *name;
	uint id;

	mp_string_dict_start(in)
		mp_string_dict_case("InstanceId", id = mp_get_uint(in))
		mp_string_dict_case("EventName", name = mp_get_string(in))
// 		mp_string_dict_case("Params", )		// there are no params!
	mp_string_dict_end(return Tv(-1));

	int rc = plugin_handle_event(id, name);
	if (rc < 0) {
		mp_put_err(out, rc);
		cw_pack_nil(out);
		return Tv(-1);
	}

	cw_pack_nil(out);		// no error
	cw_pack_map_size(out, 2);
	mp_put_lit(out, "EventId");
	cw_pack_unsigned(out, 1);
	mp_put_lit(out, "Data");
	cw_pack_map_size(out, 2);
	mp_put_lit(out, "Method");
	mp_put_lit(out, "kong.request.set_header");
	mp_put_lit(out, "Args");
	cw_pack_array_size(out, 2);
	mp_put_lit(out, "x-hello");
	mp_put_lit(out, "hello from C");
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

