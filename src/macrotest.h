#pragma once

#ifdef TEST

#define begin_test(name)	int main () {
#define end_test	fprintf(stderr, "OK\n"); return 0; }
#define do_test(msg)	\
	for(int __k = (fprintf(stderr, msg) && 0); \
		__k == 0; __k = (fprintf(stderr, "   OK\n"), 1))


#else		// no TEST

#define begin_test(name)	static int __no_call__() {
#define end_test	return 0; }
#define do_test(msg)	if(0)

#endif		// TEST
