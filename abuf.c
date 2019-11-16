#include "abuf.h"
#include <stdlib.h>
#include <string.h>

void ab_append(abuf *buf, const char *s, int len)
{
    char *new = realloc(buf->data, buf->len + len);
    if (new == NULL)
        return;
    memcpy(&new[buf->len], s, len);
    buf->data = new;
    buf->len += len;
}

void ab_free(abuf *buf)
{
    free(buf->data);
}