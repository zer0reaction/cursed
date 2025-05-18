#ifndef BUFFER_H_
#define BUFFER_H_

#include <stdbool.h>
#include <stdint.h>

enum Buffer_Mode {
    NORMAL_MODE, INSERT_MODE, REGION_MODE
};
typedef enum Buffer_Mode Buffer_Mode;

typedef struct Buffer Buffer;
struct Buffer {
    Buffer_Mode mode;
    char *data;
    char *path;
    char *reg_begin, *reg_end;
    size_t size, capacity;
    uint32_t line, col;
    uint32_t line_off;
    uint32_t col_max;
    uint32_t reg_begin_line, reg_begin_col;
    bool saved;
};

Buffer *buf_create_from_file(const char *path);
Buffer *buf_create_empty(const char *path);
Buffer *buf_open(const char *path);
void buf_save(Buffer *b);
void buf_kill(Buffer *b);

#endif /* BUFFER_H_ */
