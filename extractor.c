#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <regex.h>

#include <gumbo.h>
#include <friso/friso_API.h>
#include <friso/friso.h>

#define FRISO_PATH "friso/friso.ini"

#define MIN_CONTENT_LEN 26
#define MAX_TITLE_LEN 1024
#define MAX_PAGE_LEN  64*1024*1024


static int find_title(const GumboNode* root, char *title, size_t length);
static void utf8_count(const char *buffer, size_t *purelen, size_t *len);
static void extract_text(GumboNode* node, char *buffer, size_t *pos);
void extract(GumboOutput* Gout, char* content_buffer);

static int find_title(const GumboNode* root, char *title, size_t length)
{
        assert(root->type == GUMBO_NODE_ELEMENT);
        assert(root->v.element.children.length >= 2);

        const GumboVector* root_children = &root->v.element.children;
        GumboNode* head = NULL;
        for (int i = 0; i < root_children->length; ++i) {
                GumboNode* child = root_children->data[i];
                if (child->type == GUMBO_NODE_ELEMENT &&
                    child->v.element.tag == GUMBO_TAG_HEAD) {
                        head = child;
                        break;
                }
        }
        assert(head != NULL);

        GumboVector* head_children = &head->v.element.children;
        for (int i = 0; i < head_children->length; ++i) {
                GumboNode* child = head_children->data[i];
                if (child->type == GUMBO_NODE_ELEMENT &&
                    child->v.element.tag == GUMBO_TAG_TITLE) {
                        if (child->v.element.children.length != 1) {
                                return 0;
                        }

                        GumboNode* title_text = child->v.element.children.data[0];
                        assert(title_text->type == GUMBO_NODE_TEXT || title_text->type == GUMBO_NODE_WHITESPACE);
                        strncpy(title, title_text->v.text.text, length);

                        return 1;
                }
        }
        return 0;
}

static void utf8_count(const char *buffer, size_t *purelen, size_t *len)
{

        *purelen = *len = 0;
        while(*buffer != '\0')
        {
                if((*buffer & (char)0x80) == 0){
                        //regard continuous alpha & digit as one word
                        if(isalpha(*buffer) && isdigit(*buffer)){
                                while(isalpha(*buffer) && isdigit(*buffer)){
                                        buffer++;
                                        (*len)++;
                                }
                                (*purelen)++;
                        }
                        else{
                                buffer++;
                                (*len)++;
                        }
                } else {
                        int nbit = 0;
                        char temp = *buffer;
                        while((temp & (char)0x80) != 0){
                                nbit++;
                                temp = temp << 1;
                        }

                        buffer += nbit;
                        (*len)+=nbit;
                        (*purelen)++;
                }
        }

}

static void extract_text(GumboNode* node, char *buffer, size_t *pos)
{
        if (node->type == GUMBO_NODE_TEXT) {
                size_t purelen, len;

                utf8_count(node->v.text.text, &purelen, &len);

                //ignore short lines
                if(purelen >= MIN_CONTENT_LEN && (len + (*pos - *buffer) <= MAX_PAGE_LEN - 1)){
                        regex_t preg;

                        assert(!regcomp(&preg, "<[^>]*(>[^<>]*<)*/[^>]*>", REG_EXTENDED|REG_ICASE|REG_NOSUB));
                        if(REG_NOMATCH == regexec(&preg, node->v.text.text, 0, NULL, 0)){
                                strcpy(buffer+*pos, node->v.text.text);
                                *pos += len;
                        }
                        regfree(&preg);

                }

        } else if (node->type == GUMBO_NODE_ELEMENT &&
                   node->v.element.tag != GUMBO_TAG_SCRIPT &&
                   node->v.element.tag != GUMBO_TAG_STYLE) {

                int i;
                GumboVector* children = &node->v.element.children;
                for (i = 0; i < children->length; ++i) {
                        int prev_pos = *pos;

                        extract_text((GumboNode *)children->data[i], buffer, pos);
                        if (i != 0 && (*pos - prev_pos) > 0) {
                                buffer[*pos]='\n';
                                buffer[*pos + 1] = '\0';
                                (*pos)++;
                        }
                }

        }
}

void extract(GumboOutput* Gout, char* content_buffer)
{
	friso_t friso;
	friso_config_t config;
	friso_task_t task;

        char title[MAX_TITLE_LEN];
        size_t pos;

        if(find_title(Gout->root, title, MAX_TITLE_LEN - 1)){
                title[MAX_TITLE_LEN] = '\0';
                printf("title: %s\n", title);
        } else {
                printf("no title\n");
        }

        pos = 0;
        extract_text(Gout->root, content_buffer, &pos);
        content_buffer[pos] = '\0';

        printf("content: %s\n", content_buffer);
        getchar();

	friso = friso_new();
	config = friso_new_config();

	if(friso_init_from_ifile(friso, config, FRISO_PATH) != 1) {
		printf("fail to initialize friso and config.");
                friso_free_config(config);
                friso_free(friso);
                return;
	}

	task = friso_new_task();

	friso_set_text(task, content_buffer);
        puts("result: \n");

	while((config->next_token(friso, config, task)) != NULL){
                printf("%s ", task->token->word );
	}

        getchar();

        friso_free_task( task );

        friso_free_config(config);
        friso_free(friso);

}

