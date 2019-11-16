#include "clist.h"
#include <stdlib.h>
#include <string.h>

void clist_init(struct clist *clist)
{
    clist->size = 0;
    clist->head = NULL;
    clist->tail = NULL;
}

void clist_add_node(struct clist *clist, struct node *node)
{
    if (clist->head == NULL)
    {
        clist->head = node;
        clist->tail = node;
        node -> next = node;
    }
    else
    {
        clist->tail->next = node;
        clist->tail = node;
        node->next = clist->head;
    }
    clist -> size++;
}

void clist_add(struct clist *clist, char *data, int size)
{
    struct node *newnode = malloc(sizeof(struct node));
    newnode -> data = malloc(size + 1);
    memcpy(newnode -> data, data, size);
    newnode -> data[size] = '\0';
    newnode -> size = size;
    newnode -> next = NULL;
    clist_add_node(clist, newnode);
}

void clist_remove(struct clist *clist, struct node *node)
{
    struct node **indirect = & (clist -> head);

    while ((*indirect) != node)
    {
        indirect = &((*indirect) -> next);
    }

    *indirect = node -> next;
}
void clist_free(struct clist *clist)
{
    struct node *cur = clist -> head -> next;
    while (cur != NULL && cur != clist -> head)
    {
        struct node *save = cur;
        cur = cur -> next;
        free(save);
    }
    if (cur)
        free(cur);
}