#define _DEFAULT_SOURCE
#define _BSD_SOURCE
#define _GNU_SOURCE

#include <unistd.h>
#include <termios.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <ctype.h>
#include <pthread.h>

#include "stamina.h"
#include "abuf.h"
#include "terminal.h"

#define CTRL_KEY(k) ((k)&0x1f)
#define ALLOWED_FAILS 3

/* DATA */
struct config Config;
struct game_set Game;
pthread_mutex_t mutex;

/* ROWS */
void row_init(struct row *row, char *word, int len)
{
    row->wsize = len;
    row->spaces = 0;
    row->word = malloc(len + 1);
    row->render = malloc(len + 1);
    memcpy(row->word, word, len);
    row->word[len] = '\0';
    memcpy(row->render, word, len);
    row->render[len] = '\0';
}

void row_free(struct row *row)
{
    free(row->word);
    free(row->render);
}

void row_update(struct row *row) // TODO - remake row structre in order to get rid of malloc/free calls
{
    free(row->render);
    row->render = malloc(row->wsize + row->spaces + 1);
    memset(row->render, ' ', row->spaces);
    memcpy(row->render + row->spaces, row->word, row->wsize);
    row->render[row->spaces + row->wsize] = '\0';
}

void row_replace_at(int at, char *data, int len)
{
    row_free(Config.rows + at);
    row_init(Config.rows + at, data, len);
}

void row_replace_and_move(int at)
{
    row_replace_at(at, Config.cur_word->data, Config.cur_word->size);
    Config.cur_word = Config.cur_word->next;
}

void rows_move()
{
    for (int i = 0; i < Config.nrows; i++)
    {
        Config.rows[i].spaces += rand() % 2 + 1;
        row_update(Config.rows + i);
    }
}

void rows_replace(char *word)
{
    for (int i = 0; i < Config.nrows; i++)
    {
        struct row *row = Config.rows + i;
        if (strcmp(row->word, word) == 0)
        {
            row_replace_and_move(i);
            return;
        }
    }
}

void rows_check_and_replace()
{
    for (int i = 0; i < Config.nrows; i++)
    {
        struct row *row = Config.rows + i;
        if (row->spaces + row->wsize >= Config.screencols)
        {
            Game.fail++;
            row_replace_and_move(i);
        }
    }
}

/* FILE I/O */

void load_words(char *filename)
{
    FILE *fp = fopen(filename, "r");
    if (fp == NULL)
    {
        die("fopen");
    }

    char *line = NULL;
    int linelen = 0;
    size_t linecap = 0;

    while ((linelen = getline(&line, &linecap, fp)) != -1)
    {
        while (linelen > 0 && (line[linelen - 1] == '\n' || line[linelen - 1] == '\r' ||
                               line[linelen - 1] == ' '))
            linelen--;
        if (linelen > 0)
            clist_add(Config.dict, line, linelen);
        free(line);
        line = NULL;
    }
    fclose(fp);
}

int load_rows()
{
    if (Config.rows != NULL)
        free(Config.rows);

    Config.rows = calloc(Config.nrows, sizeof(struct row));
    if (Config.rows == NULL)
        die("calloc");

    struct node *cur = Config.cur_word == NULL ? Config.dict->head : Config.cur_word;

    if (cur == NULL)
    {
        return -1;
    }

    for (int i = 0; i < Config.nrows; i++)
    {
        row_init(Config.rows + i, cur->data, cur->size);
        cur = cur->next;
    }

    Config.cur_word = cur;
}
/* */
void draw_input_bar(abuf *ab)
{
    ab_append(ab, "\x1b[7m", 4);
    ab_append(ab, "\x1b[K", 3);
    char *buf = malloc(Config.screencols);
    memset(buf, ' ', Config.screencols);
    int inplen = Config.buflen;
    if (inplen > Config.screencols)
        inplen = Config.screencols;
    mempcpy(buf, Config.buffer, inplen);
    if (Config.screencols > inplen)
        buf[Config.screencols - 2] = '0' + Game.fail;
    ab_append(ab, buf, Config.screencols);
    ab_append(ab, "\x1b[m", 3);

    free(buf);
}

void draw_raws(abuf *ab)
{
    int i = 0;
    while (i * 2 < Config.screenrows - 1)
    {
        struct row *row = Config.rows + i;
        ab_append(ab, row->render, row->wsize + row->spaces);
        ab_append(ab, "\r\n\r\n", 4);
        i++;
    }
}

void refresh_screen()
{
    abuf ab = ABUF_INIT;

    // ab_append(&ab, "\x1b[0;0H", 6);
    ab_append(&ab, "\x1b[2J", 4);

    pthread_mutex_lock(&mutex);
    draw_raws(&ab);
    draw_input_bar(&ab);
    pthread_mutex_unlock(&mutex);
    char buf[32];
    int len = snprintf(buf, 32, "\x1b[%d;%dH", Config.nrows, Config.cx);
    ab_append(&ab, buf, len);

    write(STDOUT_FILENO, ab.data, ab.len);
    ab_free(&ab);
}

/* */
void delete_at(int at)
{
    if (at >= Config.buflen || at < 0)
        return;
    memmove(Config.buffer + at, Config.buffer + at + 1, Config.buflen - at);
    Config.buflen--;
    Config.buffer[Config.buflen] = '\0';
}

/* */
void move_cursor_right()
{
    if (Config.cx + 1 < Config.screencols)
        Config.cx++;
}

/* */
int read_key()
{
    int nread;
    char c;
    while ((nread = read(STDIN_FILENO, &c, 1)) != 1)
        if (nread == -1 && errno != EAGAIN)
            die("read");

    return c;
}

int process_key_press()
{
    int c = read_key();

    if (c == '\x1b')
    {
        read(STDIN_FILENO, &c, 2);
    }
    else if (c == DEL_KEY || c == BACKSPACE)
    {
        if (c == DEL_KEY)
            move_cursor_right();
        delete_at(Config.cx - 2);
        if (Config.cx != 0)
            Config.cx--;
    }
    else if (c == CTRL_KEY('q'))
    {
        exit(0);
    }
    else if (c == ' ')
    {
        pthread_mutex_lock(&mutex);
        rows_replace(Config.buffer);
        pthread_mutex_unlock(&mutex);
        Config.buffer[0] = '\0';
        Config.buflen = 0;
        Config.cx = 1;
    }
    else if (!iscntrl(c) && c < 128 && Config.buflen < BUFSIZE)
    {
        Config.buffer[Config.buflen] = c;
        Config.buflen++;
        Config.buffer[Config.buflen] = '\0';
        Config.cx++;
    }
}

/* THREAD FUNTIONS */
void *draw()
{
    while (Game.fail < ALLOWED_FAILS)
    {
        for (int i = 0; i < 10; i++)
        {
            refresh_screen();
            usleep((10 - Game.speed) * 10000);
        }
        
        pthread_mutex_lock(&mutex);
        rows_check_and_replace();
        rows_move();
        pthread_mutex_unlock(&mutex);

        usleep(10000);
    }
    pthread_exit(NULL);
}

void *keypress()
{
    while (Game.fail < ALLOWED_FAILS)
    {
        process_key_press();
    }
    pthread_exit(NULL);
}

/* */
void init()
{
    Config.cx = 1;
    Config.buflen = 0;
    memset(Config.buffer, '\0', BUFSIZE);
    Config.dict = malloc(sizeof(struct clist));
    clist_init(Config.dict);
    load_words("dictionary");

    if (get_window_size(&Config.screenrows, &Config.screencols) == -1)
        die("get_window_size");

    Config.nrows = Config.screencols / 2;

    if (load_rows() == -1)
        die("load_rows");

    Game.speed = 9;
    Game.fail = 0;
    Game.start = 0;
    Game.syms = 0;
}

int main()
{
    init();
    enable_raw_mode();

    pthread_t draw_thread, keypress_thread;
    pthread_create(&draw_thread, NULL, draw, NULL);
    pthread_create(&keypress_thread, NULL, keypress, NULL);

    pthread_detach(draw_thread);
    pthread_join(keypress_thread, NULL);

    return 0;
}