void die(const char *s);
void disable_raw_mode();
void enable_raw_mode();
int get_cursor_position(int *c, int *r);
int get_window_size(int *r, int *c);