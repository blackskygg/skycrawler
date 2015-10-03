#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <gumbo.h>
#include <curl/curl.h>

#include "data_struct/queue.h"
#include "data_struct/set.h"

#define MAX_URL_LEN  16*1024
#define MAX_PAGE_LEN 64*1024*1024
#define MAX_URL_NUM  100000

#define CONNECTTIMEOUT_MS 1000
#define TIMEOUT_MS        2000

#define SOURCE_FILE "sources.txt"
#define END_FLAG    "end"

#define WORKING_QUEUE "working_queue"
#define VISITED_SET   "visited_set"

typedef struct{
        char*  page;
        size_t size;
}page_t;

void search_for_links(GumboNode* node);
void process_page(page_t *page);
void crawl();
void initialize();
void release();
size_t write_data(void *buffer, size_t size, size_t nmemb, void *userp);

void initialize()
{
        char url[MAX_URL_LEN];
        FILE *fsource = fopen(SOURCE_FILE, "r");

        if((0 != init_queue()) || (0 != init_set()))
                exit(EXIT_FAILURE);

        curl_global_init(CURL_GLOBAL_ALL);

        for(;;){
                fscanf(fsource, "%s", url);
                if(strcmp(url, END_FLAG)){
                        add_to_set(url, VISITED_SET);
                        enqueue(url, WORKING_QUEUE);
                }else{
                        break;
                }
        }
        fclose(fsource);

        atexit(release);
}

void release()
{
        curl_global_cleanup();
        close_queue();
        close_set();
        puts("exited normally\n");
}


void search_for_links(GumboNode* node)
{
        GumboAttribute* href;
        GumboVector* children;

        if (node->type != GUMBO_NODE_ELEMENT) {
                return;
        }

        if (node->v.element.tag == GUMBO_TAG_A &&
            (href = gumbo_get_attribute(&node->v.element.attributes, "href"))) {
                if(!lookup_set(href->value, VISITED_SET)){
                        if(strstr(href->value, "http://") == href->value ||
                           strstr(href->value, "https://") == href->value) {
                                add_to_set(href->value, VISITED_SET);
                                enqueue(href->value, WORKING_QUEUE);
                        }
                }
        }

        children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
                search_for_links((GumboNode*)(children->data[i]));
        }
}

void process_page(page_t *page)
{
        GumboOutput* output = gumbo_parse(page->page);
        search_for_links(output->root);
        gumbo_destroy_output(&kGumboDefaultOptions, output);
}

size_t write_data(void *buffer, size_t size, size_t nmemb, void *page)
{
        size_t total;

        total = size*nmemb;
        memcpy(((page_t *)page)->page+((page_t *)page)->size, buffer, total);
        ((page_t *)page)->size += total;

        return total;

}
void crawl()
{
        char url[MAX_URL_LEN];
        page_t page;
        CURL *curl;

        page.page = malloc(MAX_PAGE_LEN);

        curl  = curl_easy_init();
        //       curl_easy_setopt(curl, CURLOPT_VERBOSE, 1L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &page);
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, CONNECTTIMEOUT_MS);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, TIMEOUT_MS);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        while(sizeof_queue(WORKING_QUEUE) > 0 && sizeof_set(VISITED_SET) <= MAX_URL_NUM){
                dequeue(url, WORKING_QUEUE);

                page.size = 0;
                curl_easy_setopt(curl, CURLOPT_URL, url);
                page.page[page.size] = '\0';
                curl_easy_perform(curl);
                process_page(&page);

        }

        curl_easy_cleanup(curl);
}

int main()
{
        initialize();
        crawl();
        release();

        return 0;

}
