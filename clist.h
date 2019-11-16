struct node
{
    int size;
    char *data;
    struct node *next;    
};

struct clist
{
    int size;
    struct node *head, *tail;
};

void clist_init(struct clist *clist);
void clist_insert(struct clist *clist, char *data, int size);
void clist_add(struct clist *clist, char *data, int size);
void clist_remove(struct clist *clist, struct node *node);
void clist_free(struct clist *clist);


