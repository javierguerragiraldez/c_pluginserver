#pragma once

#ifdef TEST

#define begin_test(name)	int main () {
#define end_test	fprintf(stderr, "OK\n"); return 0; }



#else		// no TEST

#define begin_test(name)	static int __no_call__() {
#define end_test	return 0; }

#endif		// TEST
