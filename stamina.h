#include "clist.h"

#define BUFSIZE 128

enum
{
    BACKSPACE = 127,
    DEL_KEY = 1000
};

struct row
{
    int wsize;
    int spaces;
    char *word;
    char *render;
};

struct config
{
    int screencols;
    int screenrows;
    int buflen;
    int nrows;
    int cx;
    struct row *rows;
    struct clist *dict;
    struct node *cur_word;
    char buffer[BUFSIZE];
};