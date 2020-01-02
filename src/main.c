#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <libdill.h>

#include "errtrace.h"

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


coroutine void client_connect(int port) {
	struct dill_ipaddr addr;
	int rc = dill_ipaddr_local(&addr, NULL, port, 0);
	if (rc < 0) return Tv(none);

	int s = dill_tcp_connect(&addr, now() + 1000);
	if (s < 0) return Tv(none);

	do_client(s);
}

coroutine void listen_port(int port) {
	struct dill_ipaddr addr;
	int rc = dill_ipaddr_local(&addr, NULL, port, 0);
	if (rc < 0) return Tv(none);

	int ls = dill_tcp_listen(&addr, 10);
	if (ls < 0) return Tv(none);

	do_listen(ls);
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
		if (dill_bundle_go(b, listen_port(opts.listen_port)) < 0)
			return Tv(3);
	}

	bundle_wait(b, -1);

	return 0;
}

