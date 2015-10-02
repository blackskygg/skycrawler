CFLAGS := `pkg-config --cflags gumbo hiredis` \
	  `curl-config --cflags`
STATIC_LIB_FLAGS := `pkg-config --libs gumbo hiredis` \
	           `curl-config --libs`
SHARED_LIB_FLAGS := -Wl,-rpath=/usr/local/lib

all:
	cc $(CFLAGS) $(STATIC_LIB_FLAGS) $(SHARED_LIB_FLAGS)  *.c -o crawler -g
