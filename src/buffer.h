#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdbool.h>
#include <stddef.h>

enum Buffer_Mode {
    NORMAL_MODE, INSERT_MODE, REGION_MODE
};
typedef enum Buffer_Mode Buffer_Mode;

// @perf add cur_line not to calculate it every update
// @feat add minibuf
typedef struct Buffer Buffer;
struct Buffer {
    Buffer_Mode mode;
    char *data;
    char *path;
    char *reg_begin, *reg_end;
    size_t size, capacity;
    int line, col;
    int line_off;
    int col_max;
    int reg_begin_line, reg_begin_col;
    int width, height;
    bool saved;
};

// @cleanup move functionality into buf_open
Buffer *buf_create_from_file(const char *path, int width, int height);
Buffer *buf_create_empty(const char *path, int width, int height);

Buffer *buf_open(const char *path, int width, int height);
void buf_set_dimensions(Buffer *b, int width, int height);
void buf_save(Buffer *b);
void buf_kill(Buffer *b);

#endif // BUFFER_H_
