#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gumbo.h>
#include <curl/curl.h>

#include "working_queue.h"
#include "visited_set.h"

#define MAX_URL_LEN 1024

#define SOURCE_FILE "sources.txt"
#define END_FLAG "end"

void process_page(char *url);
void crawl();
void initialize();
void release();


void initialize()
{
        char url[MAX_URL_LEN];
        FILE *fsource = fopen(SOURCE_FILE, "r");

        if((0 != init_queue()) || (0 != init_set()))
                exit(EXIT_FAILURE);

        for(;;){
                fscanf(fsource, "%s", url);
                if(strcmp(url, END_FLAG)){
                        enqueue(url);
                }else{
                        break;
                }
        }

        fclose(fsource);

}

void release()
{
        close_queue();
        close_set();
}

void process_page(char *url)
{
}

void crawl()
{
        char url[MAX_URL_LEN];
        CURL *curl;

        curl  = curl_easy_init();

        while(sizeof_queue() > 0){
                dequeue(url);
                process_page(url);

        }
}

int main()
{
        initialize();
        crawl();
        release();

        return 0;

}
