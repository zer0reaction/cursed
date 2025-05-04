#ifndef BUFFER_H_
#define BUFFER_H_

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
    size_t line, col;
    size_t line_off;
    size_t col_max;
    size_t reg_begin_line, reg_begin_col;
    bool saved;
};

#endif /* BUFFER_H_ */
