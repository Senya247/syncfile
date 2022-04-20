#ifndef LLIST_H
#define LLIST_H

#include "helper.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef NAME_MAX
#define NAME_MAX 512
#endif

struct filedata {
    struct stat st;
    char filename[NAME_MAX];
};

struct node {
    struct filedata data;
    struct node* next;
};

static void* xmalloc(size_t size)
{
    void* ptr = NULL;
    if ((ptr = malloc(size)) == NULL) {
        perror("(xmalloc) malloc");
        exit(EXIT_FAILURE);
    }

    return ptr;
}

static struct node* create_node(void)
{
    return (struct node*)xmalloc(sizeof(struct node));
}

static struct node* add_list(struct node** head, struct node* n)
{
    //    struct node *new = xmalloc(sizeof(struct node));
    //    memcpy(new, &n, sizeof(struct node));

    n->next = *head;
    *head = n;
    return n;
}

static void free_list(struct node** head)
{
    struct node* current = *head;
    struct node* next = NULL;

    while (current != NULL) {
        next = current->next;
        free(current);
        current = next;
    }

    *head = NULL;
}

static void print_times(int c, int times)
{
    for (; times; times--)
        putc(c, stdout);
}

static struct node* print_list(struct node* head)
{
    struct node* traverse = head;
    int count = 0;
    while (traverse->next) {

        printf("%s\n", traverse->data.filename);
        traverse = traverse->next;
        count++;
    }
    return head;
}

// reverse linked list
// not static
static void reverse_list(struct node** head)
{
    struct node* prev = NULL;
    struct node* current = *head;
    struct node* next = NULL;
    while (current != NULL) {
        // Store next
        next = current->next;

        // Reverse current node's pointer
        current->next = prev;

        // Move pointers one position ahead.
        prev = current;
        current = next;
    }
    *head = prev;
}

static int fill_list(struct node** head, const char* dirname)
{
    DIR* dir;
    struct dirent* de;
    int numfiles = 0;

    if ((dir = opendir(dirname)) == NULL) {
        perror("(fill_list) opendir");
        free_list(head);
        exit(EXIT_FAILURE);
    }
    while ((de = readdir(dir))) {
        // Don't add entries for parent dir and current fir
        if (!strcmp(de->d_name, ".") || !strcmp(de->d_name, ".."))
            continue;

        struct node* new_node = create_node();
        strncpy(new_node->data.filename, de->d_name, NAME_MAX);
        stat(de->d_name, &(new_node->data.st));

        add_list(head, new_node);
        numfiles++;
    }
    // Not closing causes mem leak, legit took 6 hours to fix
    closedir(dir);
    return numfiles;
}
#endif // LLIST_H
