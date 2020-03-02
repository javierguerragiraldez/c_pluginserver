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

static void usage() {
	fprintf(stderr, "c_pluginserver <port>");
}

static struct {
	int server_port;
	int listen_port;
} opts;

int parse_opts(int argc, char * const argv[]) {
	opts = (typeof(opts)){-1, -1};
	int o;

	while ((o = getopt(argc, argv, "c:s:")) != -1) {
		switch (o) {
			case 'c':
				opts.server_port = atoi(optarg);
				break;

			case 's':
				opts.listen_port = atoi(optarg);
				break;

			default:
				usage();
				return -1;
		}
	}

	return 0;
}


dill_coroutine void client_connect(int port) {
	struct dill_ipaddr addr;
	int rc = dill_ipaddr_local(&addr, NULL, port, 0);
	if (rc < 0) return Tv(none);

	int s = dill_tcp_connect(&addr, dill_now() + 1000);
	if (s < 0) return Tv(none);

// 	do_client(s);
}

dill_coroutine void instream_mdump(int s) {
	cw_unpack_context *ctx = mp_unpack_ctx(s);
	if (ctx == NULL) {
		dill_hclose(s);
		return Tv(none);
	}

	while (true) {
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

dill_coroutine void listen_port(int b, int port) {
	struct dill_ipaddr addr;
	int rc = dill_ipaddr_local(&addr, NULL, port, 0);
	if (rc < 0) return Tv(none);

	int ls = dill_tcp_listen(&addr, 10);
	if (ls < 0) return Tv(none);

	while (true) {
		int s = dill_tcp_accept(ls, NULL, -1);
		if (s < 0) return Tv(none);

		int mp = mp_attach(s, 0, 1000);
		if (mp < 0) {
			dill_hclose(s);
			Tcont();
		}

		rc = dill_bundle_go(b, instream_mdump(mp));
		if (rc != 0) {
			dill_hclose(mp);
			Tcont();
		}
	}
}


int main(int argc, char *argv[]) {
	if (argc != 2) {
		usage();
		return 1;
	}

	if (parse_opts(argc, argv) != 0) {
		return 1;
	}

	int b = dill_bundle();

	if (opts.server_port >= 0) {
		if(dill_bundle_go(b, client_connect(opts.server_port)) < 0)
			return Tv(2);
	}

	if (opts.listen_port >= 0) {
		if (dill_bundle_go(b, listen_port(b, opts.listen_port)) < 0)
			return Tv(3);
	}

	dill_bundle_wait(b, -1);

	return 0;
}

