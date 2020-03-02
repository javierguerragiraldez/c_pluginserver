#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define DILL_DISABLE_RAW_NAMES
#include <libdill.h>

#include "errtrace.h"
#include "mpack/prot.h"
#include "cwpack/cwpack.h"
#include "req_dispatch.h"

static void usage() {
	fprintf(stderr, "c_pluginserver <port>\n");
}

static struct {
	const char *connect_to;
	const char *listen_on;
} opts;

int parse_opts(int argc, char * const argv[]) {
	opts = (typeof(opts)){NULL, NULL};
	int o;

	while ((o = getopt(argc, argv, "c:s:")) != -1) {
		switch (o) {
			case 'c':
				opts.connect_to = strdup(optarg);
				if (!opts.connect_to)
					return Tv(-1);
				break;

			case 's':
				opts.listen_on = strdup(optarg);
				if (!opts.listen_on)
					return Tv(-1);
				break;

			default:
				usage();
				return -1;
		}
	}

	return 0;
}


dill_coroutine void client_connect(const char *sk_path) {
	int s = dill_ipc_connect(sk_path, 10);
	if (s < 0) return Tv(none);

// 	do_client(s);
}

dill_coroutine void session_loop(int s) {
	cw_unpack_context *ctx_in = mp_unpack_ctx(s);
	if (!ctx_in) Tf(end);
	cw_pack_context *ctx_out = mp_pack_ctx(s);
	if (!ctx_out) Tf(end);

	while (true) {
		int rc = dispatch_request(ctx_in, ctx_out);
		if (rc < 0) break;
	}

end:
	dill_hclose(s);
}

dill_coroutine void instream_mdump(int s) {
	cw_unpack_context *ctx = mp_unpack_ctx(s);
// 	fprintf(stderr, "ctx: (%p) [%p, %p, %p]\n", ctx, ctx->current, ctx->start, ctx->end);
	if (ctx == NULL) {
		dill_hclose(s);
		return Tv(none);
	}

	while (true) {
// 		fprintf(stderr, "want to decode something\n");
		cw_unpack_next(ctx);
		if (errno != 0) {
			dill_hclose(s);
			return Tv(none);
		}
		const cwpack_item *item = &ctx->item;

		switch (item->type) {
			case CWP_ITEM_NIL:
				printf("nil\n");
				break;

			case CWP_ITEM_BOOLEAN:
				printf("boolean: %d\n", item->as.boolean);
				break;

			case CWP_ITEM_POSITIVE_INTEGER:
				printf("posint: %lu\n", item->as.u64);
				break;

			case CWP_ITEM_NEGATIVE_INTEGER:
				printf("negint: %ld\n", item->as.i64);
				break;

			case CWP_ITEM_FLOAT:
				printf("float: %g\n", item->as.real);
				break;

			case CWP_ITEM_DOUBLE:
				printf("double: %g\n", item->as.long_real);
				break;

			case CWP_ITEM_STR:
				printf("str: %-*.*s\n",
					   item->as.str.length, item->as.str.length, (const char*)item->as.str.start);
				break;

			case CWP_ITEM_BIN:
				printf("bin: %-*.*s\n",
					   item->as.bin.length, item->as.bin.length, (const char*)item->as.bin.start);
				break;

			case CWP_ITEM_ARRAY:
				printf("array: # %d\n", item->as.array.size);
				break;

			case CWP_ITEM_MAP:
				printf("map: # %d\n", item->as.map.size);
				break;

			case CWP_ITEM_EXT:
				printf("ext: len=%d\n", item->as.ext.length);
				break;

			case CWP_NOT_AN_ITEM:
				printf("not an item\n");
				errno = ENOMSG;
				return Tv(none);
				break;

			default:
				if (item->type >= CWP_ITEM_MIN_USER_EXT &&
					item->type <= CWP_ITEM_MAX_USER_EXT
				) {
					printf ("user ext: %d, len=%d\n", item->type, item->as.ext.length);

				} else if (item->type >= CWP_ITEM_MIN_RESERVED_EXT &&
					item->type <= CWP_ITEM_MAX_RESERVED_EXT
				) {
					printf ("reseved ext: %d, len=%d\n", item->type, item->as.ext.length);

				} else {
					printf("unknown type %d\n", item->type);
				}
				break;
		}
	}
}

dill_coroutine void listen_port(int b, const char *sk_path) {
	int ls = dill_ipc_listen(sk_path, 10);
	if (ls < 0) return Tv(none);

	while (true) {
		int s = dill_ipc_accept(ls, -1);
		if (s < 0) return Tv(none);

		int mp = mp_attach(s, 0, 1000);
		if (mp < 0) {
			dill_hclose(s);
			Tcont();
		}

		int rc = dill_bundle_go(b, session_loop(mp));
		if (rc != 0) {
			dill_hclose(mp);
			Tcont();
		}
	}
}


int main(int argc, char *argv[]) {
// 	if (argc != 2) {
// 		usage();
// 		return 1;
// 	}

	if (parse_opts(argc, argv) != 0) {
		return 1;
	}

	int b = dill_bundle();

	if (opts.connect_to) {
		if(dill_bundle_go(b, client_connect(opts.connect_to)) < 0)
			return Tv(2);
	}

	if (opts.listen_on) {
		if (dill_bundle_go(b, listen_port(b, opts.listen_on)) < 0)
			return Tv(3);
	}

	dill_bundle_wait(b, -1);

	return 0;
}

