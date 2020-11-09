#ifndef CMD_H
#define CMD_H

#include <stdio.h>

char** cmd_split(const char* cmd);
char** cmd_get(FILE* f);
void cmd_free(char** cmd);

#endif
