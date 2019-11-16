typedef struct abuf
{
    char *data;
    int len;
} abuf;

#define ABUF_INIT \
    {             \
        NULL, 0   \
    }

void ab_append(abuf *buf, const char *s, int len);
void ab_free(abuf *buf);