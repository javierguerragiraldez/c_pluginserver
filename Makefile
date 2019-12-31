CC=gcc
CFLAGS="-Wall"

debug:clean
	$(CC) $(CFLAGS) -g -o c_pluginserver main.c
stable:clean
	$(CC) $(CFLAGS) -o c_pluginserver main.c
clean:
	rm -vfr *~ c_pluginserver
