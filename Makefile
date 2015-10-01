all:
	cc `pkg-config --cflags --libs  gumbo sqlite3` `curl-config --libs --cflags`  *.c -o crawler
