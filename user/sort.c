#include "kernel/fcntl.h"
#include "kernel/types.h"
#include "user/user.h"

#define BUFFER_SIZE 10
#define NULL 0

struct node {
    struct node *last;
    struct node *next;
    char value[BUFFER_SIZE];
};


int
main(int argc, char *argv[]) {
    int fd = 0;
    if(argc > 1) {
        if((fd = open(argv[1], O_RDONLY)) < 0){
            fprintf(2, "Cannot open %s\n", argv[1]);
            exit(1);
        }
    }

    struct node *root = malloc(sizeof(struct node));
    root->last = NULL;
    root->next = NULL;
    strcpy(root->value, "ROOT");
    struct node *last_node = root;

    char buf[BUFFER_SIZE];
    int counter = 0;
    for(;;) {
        int len = read(fd, buf+counter, 1);

        if(counter == BUFFER_SIZE - 1) {
            fprintf(2, "Buffer size exceeded\n");
            exit(1);
        }

        if(buf[counter] == ' ' || buf[counter] == '\n' || len == 0) {
            buf[counter] = '\0';

            struct node *curr_node = malloc(sizeof(struct node));
            curr_node->last = last_node;
            curr_node->next = NULL;
            strcpy(curr_node->value, buf);
            last_node->next = curr_node;
            last_node = curr_node;
            counter = 0;

            if(len == 0)
                break;
        } else {
            counter++;
        }
    }

    if(root->next) {
        struct node *tmp = root->next;
        free(root);
        root = tmp;
    }

    struct node *end = NULL;
    short swapped = 1;
    while(swapped) {
        swapped = 0;
        struct node *n;
        for(n = root; n && n->next && n->next->next != end; n = n->next) {
            if(strcmp(n->value, n->next->value) > 0) {
                char tmp_buf[BUFFER_SIZE];
                strcpy(tmp_buf, n->value);
                strcpy(n->value, n->next->value);
                strcpy(n->next->value, tmp_buf);
                swapped = 1;
            }
        }
        if(n && n->next)
            end = n->next;
    }

    struct node *curr_node = root;
    while(curr_node) {
        printf("%s ", curr_node->value);
        struct node *tmp = curr_node->next;
        free(curr_node);
        curr_node = tmp;
    }
    printf("\n");

    exit(0);
}