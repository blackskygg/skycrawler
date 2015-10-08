HIREDIS_FLAGS := `pkg-config --cflags hiredis`
GUMBO_FLAGS := `pkg-config --cflags gumbo`
CURL_FLAGS  := `curl-config --cflags`
FRISO_FLAGS := -Iusr/local/include

STATIC_LIB_FLAGS := `pkg-config --libs gumbo hiredis` \
	           `curl-config --libs` -lfriso -lpthread -lssl3
SHARED_LIB_FLAGS := -Wl,-rpath=/usr/local/lib

DEBUG := -g

all: crawler search

search: search.c search.h zset.o hash.o
	cc $(STATIC_LIB_FLAGS) $(SHARED_LIB_FLAGS) search.c zset.o hash.o -o search $(DEBUG)

crawler: main.o set.o queue.o hash.o zset.o extractor.o indexbuilder.o
	cc  $(STATIC_LIB_FLAGS) $(SHARED_LIB_FLAGS) *.o -o crawler $(DEBUG)

main.o: main.c set.o queue.o zset.o hash.o
	cc -c $(CURL_FLAGS) $(GUMBO_FLAGS) main.c -o main.o $(DEBUG)

queue.o: data_struct/queue.c data_struct/queue.h
	cc -c $(HIREDIS_FLAGS) data_struct/queue.c -o queue.o $(DEBUG)

set.o: data_struct/set.c data_struct/set.h
	cc -c $(HIREDIS_FLAGS) data_struct/set.c -o set.o $(DEBUG)

zset.o: data_struct/zset.c data_struct/zset.h
	cc -c $(HIREDIS_FLAGS) data_struct/zset.c -o zset.o $(DEBUG)

hash.o: data_struct/hash.c data_struct/hash.h
	cc -c $(HIREDIS_FLAGS) data_struct/hash.c -o hash.o $(DEBUG)

extractor.o: extractor.c extractor.h
	cc -c extractor.c -o extractor.o $(DEBUG)

indexbuilder.o: indexbuilder.h indexbuilder.c zset.o hash.o
	cc -c $(FRISO_FLAGS) indexbuilder.c -o indexbuilder.o $(DEBUG)
