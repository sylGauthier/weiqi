#ifndef UTILS_H
#define UTILS_H

int pipe_proc(const char* cmd, char* const* argv, int* in, int* out);
int read_line(int fd, char* buf, size_t size);
int write_str(int fd, const char* s);
#endif
