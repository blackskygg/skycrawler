HIREDIS_FLAGS := `pkg-config --cflags hiredis`
GUMBO_FLAGS := `pkg-config --cflags gumbo`
CURL_FLAGS  := `curl-config --cflags`

STATIC_LIB_FLAGS := `pkg-config --libs gumbo hiredis` \
	           `curl-config --libs` -lboost_regex -lstdc++
SHARED_LIB_FLAGS := -Wl,-rpath=/usr/local/lib

all: main.o set.o queue.o #extractor.o
	cc  $(STATIC_LIB_FLAGS) $(SHARED_LIB_FLAGS) *.o -o crawler

main.o: main.c
	cc -c $(CURL_FLAGS) $(GUMBO_FLAGS) main.c -o main.o

queue.o: data_struct/queue.c data_struct/queue.h
	cc -c $(HIREDIS_FLAGS) data_struct/queue.c -o queue.o

set.o: data_struct/set.c data_struct/set.h
	cc -c $(HIREDIS_FLAGS) data_struct/set.c -o set.o

#extractor.o: extractor/TextExtractor.cpp extractor/TextExtractor.h
#	g++ -c extractor/TextExtractor.cpp -o extractor.o
