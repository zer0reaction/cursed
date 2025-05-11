#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <locale.h>
#include <assert.h>

/* idk where the header is */
int mvaddwstr(int y, int x, const wchar_t *wstr);

#include "utility.h"
#include "buffer.h"

#define TAB_SPACES 4
#define KILL_BUFFER_SIZE (1024 * 1024)

char kill_buffer[KILL_BUFFER_SIZE] = {0};

/* ------------------------------------------------------------------------
Buffer functions
------------------------------------------------------------------------- */

Buffer *buf_create_from_file(const char *path) {
    Buffer *b = NULL;
    size_t size = 0;
    FILE *fp = NULL;

    fp = fopen(path, "r");
    if (fp == NULL) {
        return NULL;
    }

    fseek(fp, 0, SEEK_END);
    size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    b = malloc(sizeof(Buffer));
    memset(b, 0, sizeof(Buffer));

    b->data = malloc(size + 1);
    fread(b->data, 1, size, fp);
    b->data[size] = '\0';

    b->path = malloc(strlen(path) + 1);
    strcpy(b->path, path);

    b->saved = true;

    fclose(fp);
    return b;
}

Buffer *buf_create_empty(const char *path) {
    Buffer *b = NULL;

    b = malloc(sizeof(Buffer));
    memset(b, 0, sizeof(Buffer));

    b->data = malloc(1);
    b->data[0] = '\0';

    b->path = malloc(strlen(path) + 1);
    strcpy(b->path, path);

    b->saved = false;

    return b;
}

Buffer *buf_open(const char *path) {
    FILE *fp = NULL;

    /* TODO is fopen the right way to do this? */
    /*      and is this function abstraction even good? */

    fp = fopen(path, "r");
    if (fp == NULL) {
        return buf_create_empty(path);
    } else {
        fclose(fp);
        return buf_create_from_file(path);
    }
}

void buf_save(Buffer *b) {
    FILE *fp = NULL;
    size_t len = 0;

    append_newline_maybe(b);

    len = strlen(b->data);
    fp = fopen(b->path, "w");
    fwrite(b->data, 1, len, fp);
    b->saved = true;
    fclose(fp);
}

void buf_kill(Buffer *b) {
    free(b->data);
    free(b->path);
    free(b);
}

/* ------------------------------------------------------------------------
Editor functions
------------------------------------------------------------------------- */

void move_down(Buffer *b) {
    if (b->line < line_count(b->data) - 1) {
        b->line++;
        adjust_col(b);
    }
}

void move_up(Buffer *b) {
    if (b->line > 0) {
        b->line--;
        adjust_col(b);
    }
}

void move_right(Buffer *b) {
    char *lp = NULL;
    size_t len = 0;

    lp = line_goto(b->data, b->line);
    len = line_len(lp);
    if (b->col < len) {
        b->col++;
        b->col_max = b->col;
    }
}

void move_left(Buffer *b) {
    if (b->col > 0) {
        b->col--;
        b->col_max = b->col;
    }
}

/* TODO rename */
void move_line_left(Buffer *b) {
    b->col = b->col_max = 0;
}

void move_line_begin(Buffer *b) {
    size_t i = 0;
    char *cur_line = NULL;
    size_t len = 0;

    cur_line = line_goto(b->data, b->line);
    len = line_len(cur_line);

    for (i = 0; i < len; ++i) {
        if (cur_line[i] != ' ') {
            b->col = b->col_max = i;
            break;
        }
    }
}

void move_line_end(Buffer *b) {
    char *cur_line = NULL;
    size_t len = 0;

    cur_line = line_goto(b->data, b->line);
    len = line_len(cur_line);
    b->col = b->col_max = len;
}

/* TODO rename */
void move_screen_down(Buffer *b, unsigned short int screen_height) {
    unsigned short int n = 0;
    unsigned long int pos = 0;

    n = screen_height / 2;
    pos = b->line + n;
    b->line = (pos > line_count(b->data) - 1) ? line_count(b->data) - 1: pos;
    adjust_col(b);
}

/* TODO rename */
void move_screen_up(Buffer *b, size_t screen_height) {
    unsigned short int n = 0;
    long int pos = 0;

    n = screen_height / 2;
    pos = b->line - n;
    b->line = (pos >= 0) ? pos : 0;
    adjust_col(b);
}

/* TODO rename */
void move_screen_center(Buffer *b, size_t screen_height) {
    long int off = 0;

    off = b->line - (screen_height / 2);
    b->line_off = (off >= 0) ? off : 0;
    adjust_col(b);
}

void insert_char(Buffer *b, char c) {
    static unsigned char size = 0;
    static unsigned char acc = 0;
    static char buf[4] = {0};

    if (char_size(c) > 0) {
        size = char_size(c);
        acc = 0;
        memset(buf, 0, 4);
    }

    buf[acc++] = c;

    if (acc != 0 && acc == size) {
        size_t pos = get_current_pos(b);
        insert_substr(b, pos, buf, size);

        if (buf[0] == '\n') {
            b->col = b->col_max = 0;
            b->line++;
        } else {
            b->col++;
            b->col_max = b->col;
        }

        b->saved = false;
    }
}

void delete_char(Buffer *b) {
    size_t pos = 0;
    size_t prev_line_len = 0;

    if ((pos = get_current_pos(b)) == 0) return;
    pos--;

    if (b->line > 0) {
        char *prev_line = line_goto(b->data, b->line - 1);
        prev_line_len = line_len(prev_line);
    }

    while (char_size(b->data[pos]) == 0) pos--;

    erase_substr(b, pos, char_size(b->data[pos]));

    if (b->col > 0) {
        b->col--;
        b->col_max = b->col;
    } else {
        b->col = b->col_max = prev_line_len;
        b->line--;
    }

    b->saved = false;
}

void clear_killed(void) {
    memset(kill_buffer, 0, sizeof(kill_buffer));
}

void kill_line(Buffer *b) {
    size_t size = 0;
    size_t killed = 0;
    size_t pos = 0;
    char *cur_line = NULL;

    cur_line = line_goto(b->data, b->line);
    size = line_size(cur_line);
    pos = cur_line - b->data;
    killed = (cur_line[size] != '\0') ? size + 1 : size;

    strncat(kill_buffer, cur_line, killed);
    erase_substr(b, pos, killed);

    b->saved = false;
    adjust_col(b);
}

void clear_region(Buffer *b) {
    b->reg_begin = b->reg_end = NULL;
    b->reg_begin_line = b->reg_begin_col = 0;
}

void begin_region(Buffer *b) {
    char *cur = NULL;

    cur = b->data + get_current_pos(b);
    b->reg_begin = b->reg_end = cur;
    b->reg_begin_line = b->line;
    b->reg_begin_col = b->col;
}

void end_region(Buffer *b) {
    char *cur = NULL;

    cur = b->data + get_current_pos(b);
    b->reg_end = cur;
}

void kill_region(Buffer *b) {
    size_t reg_len = 0;

    if (b->reg_begin == b->reg_end) return;

    reg_len = b->reg_end - b->reg_begin;

    strncat(kill_buffer, b->reg_begin, reg_len);
    erase_substr(b, b->reg_begin - b->data, reg_len);

    /* TODO is this stupid? */
    b->line = b->reg_begin_line;
    b->col = b->col_max = b->reg_begin_col;

    b->saved = false;
}

void copy_region(Buffer *b) {
    size_t reg_len = 0;

    if (b->reg_begin == b->reg_end) return;

    reg_len = b->reg_end - b->reg_begin;

    strncat(kill_buffer, b->reg_begin, reg_len);

    /* TODO is this stupid? */
    b->line = b->reg_begin_line;
    b->col = b->col_max = b->reg_begin_col;
}

void paste(Buffer *b) {
    size_t len = 0;
    size_t pos = 0;
    size_t count = 0;

    len = strlen(kill_buffer);
    pos = get_current_pos(b);
    count = line_count(kill_buffer);

    if (len == 0) return;

    insert_substr(b, pos, kill_buffer, len);

    b->line += count - 1;

    if (kill_buffer[len - 1] == '\n') {
        b->col = b->col_max = 0;
    } else if (count == 1) {
        b->col += line_len(kill_buffer);
        b->col_max = b->col;
    } else {
        char *lp = NULL;

        lp = line_goto(kill_buffer, count - 1);
        b->col = b->col_max = line_len(lp);
    }

    b->saved = false;
}

/* ------------------------------------------------------------------------
Render functions
------------------------------------------------------------------------- */

#define HEIGHT (LINES - 1)
#define WIDTH (COLS)

void render_init(void) {
    initscr();
    noecho();
    cbreak();
    nl();
}

void render_end(void) {
    endwin();
}

void render(Buffer *b) {
    char *lp = NULL;
    char status[128] = {0};
    wchar_t wcbuf[1024] = {0};

    curs_set(0);

    start_color();
    init_pair(1, COLOR_BLACK, COLOR_WHITE);

    lp = line_goto(b->data, b->line_off);
    mbstowcs(wcbuf, lp, 1024);

    if (b->mode == INSERT_MODE) {
        sprintf(status, "[insert] %s", b->path);
    } else if (b->mode == REGION_MODE) {
        sprintf(status, "[region] %s", b->path);
    } else if (b->mode == NORMAL_MODE) {
        sprintf(status, "%s", b->path);
    }
    if (!(b->saved)) strcat(status, "*");
    sprintf(status + strlen(status), " (%lu)", strlen(kill_buffer));

    erase();

    attron(COLOR_PAIR(1));
    mvaddstr(0, 0, status);
    attroff(COLOR_PAIR(1));

    mvaddwstr(1, 0, wcbuf);
    move(b->line - b->line_off + 1, b->col);
    refresh();

    curs_set(1);
}

int main(int argc, char **argv) {
    int i = 0;
    Buffer *b = NULL;
    Buffer *buf_list[4] = {0};
    bool should_close = false;

    if (argc == 1 || argc > 5) return 1;

    setlocale(LC_ALL, "");

    for (i = 0; i < argc - 1; ++i) {
        buf_list[i] = buf_open(argv[i + 1]);
    }
    b = buf_list[0];

    render_init();

    while (!should_close) {
        int c = 0;

        adjust_offset(b, HEIGHT);
        render(b);
        c = getch();

        if (b->mode == NORMAL_MODE) {
            switch (c) {
                case 'q':
                    /* TODO check for all buffers to be saved */
                    if (b->saved) should_close = true;
                    break;
                case 'Q':
                    should_close = true;
                    break;
                case 'i':
                    b->mode = INSERT_MODE;
                    break;
                case 'a':
                    move_right(b);
                    b->mode = INSERT_MODE;
                    break;
                case 'I':
                    move_line_begin(b);
                    b->mode = INSERT_MODE;
                    break;
                case 'A':
                    move_line_end(b);
                    b->mode = INSERT_MODE;
                    break;
                case 'j':
                    move_down(b);
                    break;
                case 'k':
                    move_up(b);
                    break;
                case 'l':
                    move_right(b);
                    break;
                case 'h':
                    move_left(b);
                    break;
                case 's':
                    buf_save(b);
                    break;
                case '^':
                    move_line_begin(b);
                    break;
                case '$':
                    move_line_end(b);
                    break;
                case '0':
                    move_line_left(b);
                    break;
                case 'f':
                    move_screen_center(b, HEIGHT);
                    break;
                case 'd':
                    kill_line(b);
                    break;
                case 'c':
                    clear_killed();
                    break;
                case 'y':
                    paste(b);
                    break;
                case 'n':
                    move_screen_down(b, HEIGHT);
                    move_screen_center(b, HEIGHT);
                    break;
                case 'p':
                    move_screen_up(b, HEIGHT);
                    move_screen_center(b, HEIGHT);
                    break;
                case ' ':
                    begin_region(b);
                    b->mode = REGION_MODE;
                    break;
                /* Ctrl+j */
                case 10:
                    b = (buf_list[0] != NULL) ? buf_list[0] : b;
                    break;
                /* Ctrl+k */
                case 11:
                    b = (buf_list[1] != NULL) ? buf_list[1] : b;
                    break;
                /* Ctrl+l */
                case 12:
                    b = (buf_list[2] != NULL) ? buf_list[2] : b;
                    break;
                /* Ctrl+; */
                case 59:
                    b = (buf_list[3] != NULL) ? buf_list[3] : b;
                    break;
            }
        } else if (b->mode == INSERT_MODE) {
            switch (c) {
                /* escape */
                case 27:
                    b->mode = NORMAL_MODE;
                    break;
                /* backspace */
                case 127:
                case 8:
                    delete_char(b);
                    break;
                /* tab */
                case 9: {
                    int i = 0;
                    for (i = 0; i < TAB_SPACES; ++i) insert_char(b, ' ');
                } break;
                default: insert_char(b, c);
            }
        } else if (b->mode == REGION_MODE) {
            switch (c) {
                /* escape */
                case 27:
                    clear_region(b);
                    b->mode = NORMAL_MODE;
                    break;
                case ' ':
                    end_region(b);
                    b->mode = NORMAL_MODE;
                    break;
                case 'd':
                    end_region(b);
                    kill_region(b);
                    b->mode = NORMAL_MODE;
                    break;
                case 'r':
                    end_region(b);
                    copy_region(b);
                    b->mode = NORMAL_MODE;
                    break;
                case 'j':
                    move_down(b);
                    break;
                case 'k':
                    move_up(b);
                    break;
                case 'l':
                    move_right(b);
                    break;
                case 'h':
                    move_left(b);
                    break;
                case '^':
                    move_line_begin(b);
                    break;
                case '$':
                    move_line_end(b);
                    break;
                case '0':
                    move_line_left(b);
                    break;
                case 'f':
                    move_screen_center(b, HEIGHT);
                    break;
                case 'c':
                    clear_killed();
                    break;
                case 'n':
                    move_screen_down(b, HEIGHT);
                    move_screen_center(b, HEIGHT);
                    break;
                case 'p':
                    move_screen_up(b, HEIGHT);
                    move_screen_center(b, HEIGHT);
                    break;
                /* Ctrl+j */
                case 10:
                    b = (buf_list[0] != NULL) ? buf_list[0] : b;
                    break;
                /* Ctrl+k */
                case 11:
                    b = (buf_list[1] != NULL) ? buf_list[1] : b;
                    break;
                /* Ctrl+l */
                case 12:
                    b = (buf_list[2] != NULL) ? buf_list[2] : b;
                    break;
                /* Ctrl+; */
                case 59:
                    b = (buf_list[3] != NULL) ? buf_list[3] : b;
                    break;
            }
        }
    }

    render_end();

    for (i = 0; i < argc - 1; ++i) {
        buf_kill(buf_list[i]);
    }

    return 0;
}
