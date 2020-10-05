#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "utils.h"

int move_to_str(char dest[3], unsigned int row, unsigned int col) {
    if (row > 25 || col > 25) return 0;
    dest[0] = col + 'A';
    if (dest[0] >= 'I') dest[0]++;
    sprintf(dest + 1, "%d", row + 1);
    return 1;
}

int str_to_move(unsigned int* row, unsigned int* col, const char* str) {
    if (strlen(str) < 2 || str[0] < 'A' || str[0] > 'Z' || str[1] < '0' || str[1] > '9'
            || (strlen(str) == 3 && (str[2] < '0' || str[2] > '9')) || strlen(str) > 3) {
        fprintf(stderr, "Error: move must be letter and number, like D4, Q16, etc.\n");
        return 0;
    } else {
        *col = str[0] - 'A';
        if (*col >= 9) *col -= 1;
        *row = strtol(str + 1, NULL, 10);
        if (!*row) {
            fprintf(stderr, "Error: row can't be 0\n");
            return 0;
        }
        *row -= 1;
    }
    return 1;
}

char** split_cmd(const char* cmd) {
    char* split;
    char** res;
    unsigned int len, argc = 0, i, quote = 0, esc = 0, c = 0;

    len = strlen(cmd);
    if (!(split = malloc((len + 1) * sizeof(char)))) {
        return NULL;
    }
    for (i = 0; i <= len; i++) {
        if (!esc && cmd[i] == '\\') {
            esc = 1;
        } else if (esc) {
            split[c++] = cmd[i];
        } else if (cmd[i] == '"') {
            quote = !quote;
        } else if (quote) {
            split[c++] = cmd[i];
        } else if (cmd[i] == ' ') {
            split[c++] = '\0';
            argc++;
        } else {
            split[c++] = cmd[i];
        }
    }
    if (!(res = malloc((argc + 2) * sizeof(char*)))) {
        free(split);
        return NULL;
    }
    for (i = 0; i <= argc; i++) {
        res[i] = split;
        split += strlen(split) + 1;
    }
    res[argc + 1] = NULL;
    return res;
}
