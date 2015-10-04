#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <sys/stat.h>

#include <gumbo.h>
#include <friso/friso_API.h>
#include <friso/friso.h>

#define MIN_CONTENT_LEN 16

int find_title(const GumboNode* root, char *title, int length)
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

void utf8_count(const char *buffer, int *purelen, int *len)
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

void extract_text(GumboNode* node, char *buffer, int *pos){
        if (node->type == GUMBO_NODE_TEXT) {
                int purelen, len;

                utf8_count(node->v.text.text, &purelen, &len);

                //ignore short lines
                if(purelen >= MIN_CONTENT_LEN){
                        strcpy(buffer+*pos, node->v.text.text);
                        *pos += len;
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


static void read_file(FILE* fp, char** output, int* length) {
        struct stat filestats;
        int fd = fileno(fp);
        fstat(fd, &filestats);
        *length = filestats.st_size;
        *output = malloc(*length + 1);
        int start = 0;
        int bytes_read;
        while ((bytes_read = fread(*output + start, 1, *length - start, fp))) {
                start += bytes_read;
        }
}


int main(int argc, char **argv)
{
        FILE *fp = fopen(argv[1], "r");
        char *output, title[1000], content[64000];
        int  length;
        int  pos;

        char line[20480] = {0};
	int i;

        fstring path = "friso.ini", mode = NULL;

	friso_t friso;
	friso_config_t config;
	friso_task_t task;

        read_file(fp, &output, &length);
        output[length] = '\0';
        GumboOutput* Goutput = gumbo_parse(output);

        find_title(Goutput->root, title, 1000);
        title[999] = '\0';
        printf("title: %s\n", title);

        pos = 0;
        extract_text(Goutput->root, content, &pos);
//        printf("content:\n%s", content);

	//initialize friso
	friso = friso_new();
	config = friso_new_config();

	if ( friso_init_from_ifile(friso, config, path) != 1 ) {
		printf("fail to initialize friso and config.");
                goto err;
	}

	task = friso_new_task();

        //set the task text.
	friso_set_text( task, content );
        puts("result: \n");

	while ( ( config->next_token( friso, config, task ) ) != NULL ) 
	{

                printf("%s ", task->token->word );
	}



        friso_free_task( task );
err:

        friso_free_config(config);
        friso_free(friso);


        return 0;

}

