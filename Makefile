HIREDIS_FLAGS := `pkg-config --cflags hiredis`
GUMBO_FLAGS := `pkg-config --cflags gumbo`
CURL_FLAGS  := `curl-config --cflags`
FRISO_FLAGS := -Iusr/local/include

STATIC_LIB_FLAGS := `pkg-config --libs gumbo hiredis` \
	           `curl-config --libs` -lfriso
SHARED_LIB_FLAGS := -Wl,-rpath=/usr/local/lib

DEBUG := -g

all: main.o set.o queue.o extractor.o
	cc  $(STATIC_LIB_FLAGS) $(SHARED_LIB_FLAGS) *.o -o crawler $(DEBUG)

main.o: main.c
	cc -c $(CURL_FLAGS) $(GUMBO_FLAGS) main.c -o main.o $(DEBUG)

queue.o: data_struct/queue.c data_struct/queue.h
	cc -c $(HIREDIS_FLAGS) data_struct/queue.c -o queue.o $(DEBUG)

set.o: data_struct/set.c data_struct/set.h
	cc -c $(HIREDIS_FLAGS) data_struct/set.c -o set.o $(DEBUG)

extractor.o: extractor.c extractor.h
	cc -c $(FRISO_FLAGS) extractor.c -o extractor.o $(DEBUG)
