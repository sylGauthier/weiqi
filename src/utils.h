#ifndef UTILS_H
#define UTILS_H

char* strdup(char* s);
int move_to_str(char dest[3], unsigned char row, unsigned char col);
int str_to_move(unsigned char* row, unsigned char* col,
                char* pass, const char* str);
char** split_cmd(const char* cmd);

#endif
