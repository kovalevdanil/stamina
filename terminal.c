#include "terminal.h"
#include <unistd.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdlib.h>

struct termios original_set;

void die(const char *s)
{
    write(STDOUT_FILENO, "\x1b[2J", 4);
    write(STDOUT_FILENO, "\x1b[H", 3);

    perror(s);
    exit(1);
}

void disable_raw_mode()
{
    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &original_set) == -1)
        die("tcsetattr");
}

void enable_raw_mode()
{
    if (tcgetattr(STDIN_FILENO, &original_set) == -1)
        die("tcgetattr");
    atexit(disable_raw_mode);

    struct termios raw = original_set;
    raw.c_iflag &= ~(IXON | ICRNL | ISTRIP | BRKINT | INPCK); // (Carriage Return New Line)
    raw.c_oflag &= ~(OPOST);                                  // disable '\r\n' on each of new lines
    raw.c_lflag &= ~(ECHO | ICANON | ISIG | IEXTEN);
    raw.c_cflag |= CS8;

    raw.c_cc[VMIN] = 0;
    raw.c_cc[VTIME] = 5;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1)
        die("tcsetattr");
}

int get_cursor_position(int *c, int *r)
{
    char buf[32];
    int i = 0;

    if (write(STDOUT_FILENO, "\x1b[6n", 4) == -1)
        return -1;
    printf("\r\n");

    while (i < sizeof(buf) - 1)
    {
        if (read(STDIN_FILENO, &buf[i], 1) != 1)
            break;
        if (buf[i] == 'R')
            break;
        i++;
    }
    buf[i] = '\0';

    if (buf[0] != '\x1b' || buf[1] != '[')
        return -1;
    if (sscanf(&buf[2], "%d;%d", c, r) != 2)
        return -1;

    return 0;
}

int get_window_size(int *r, int *c)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0)
    {
        if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12)
            return -1;

        return get_cursor_position(r, c);
    }
    else
    {
        *c = ws.ws_col;
        *r = ws.ws_row;
        return 0;
    }
}