#ifndef MAIN_H_
#define MAIN_H_

enum Buffer_Mode {
    NORMAL, INSERT
};
typedef enum Buffer_Mode Buffer_Mode;

typedef struct Buffer Buffer;
struct Buffer {
    char *data;
    char *path;
    bool saved;
    Buffer_Mode mode;
    size_t line, col;
    size_t line_off;
    size_t col_max;
};

Buffer *buf_create_from_file(const char *path);
Buffer *buf_create_new_file(const char *path);
Buffer *buf_open(const char *path);
void buf_save(Buffer *b);
void buf_kill(Buffer *b);
void buf_append_newline_maybe(Buffer *b);
size_t buf_get_current_pos(Buffer *b);

char *line_next(char *start);
size_t line_len(char *line);
size_t line_count(Buffer *b);
char *line_goto(Buffer *b, size_t n);

void move_adjust_col(Buffer *b);
void move_down(Buffer *b);
void move_up(Buffer *b);
void move_right(Buffer *b);
void move_left(Buffer *b);
void move_screen_down(Buffer *b, unsigned short int screen_height);
void move_screen_up(Buffer *b, size_t screen_height);
void move_screen_center(Buffer *b, size_t screen_height);

void insert_char(Buffer *b, char c);
void delete_char(Buffer *b);

void render_init(void);
void render_end(void);
void render(Buffer *b);
void render_offset_adjust(Buffer *b);

#endif /* MAIN_H_ */
