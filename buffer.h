#ifndef BUFFER_H_
#define BUFFER_H_

enum Buffer_Mode {
    NORMAL_MODE, INSERT_MODE
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

#endif /* BUFFER_H_ */
