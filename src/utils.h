#ifndef UTILS_H
#define UTILS_H

int move_to_str(char dest[3], unsigned char row, unsigned char col);
int str_to_move(unsigned char* row, unsigned char* col,
                char* pass, const char* str);

int pipe_proc(const char* cmd, char* const* argv, int* in, int* out);
int read_line(int fd, char* buf, size_t size);
int write_str(int fd, const char* s);
#endif
