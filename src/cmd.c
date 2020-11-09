#include <string.h>
#include <stdlib.h>

#include "cmd.h"

char** cmd_split(const char* cmd) {
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
            while (cmd[i + 1] == ' ') i++;
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

static char* get_line(FILE* f) {
    char c, *res = NULL;
    unsigned int size = 0, incr = 256, cur = 0;

    while ((c = fgetc(f)) != EOF) {
        if (cur >= size) {
            char* tmp;
            size += incr;
            if (!(tmp = realloc(res, size))) {
                free(res);
                return NULL;
            }
            res = tmp;
        }
        if (c == '\n') break;
        res[cur++] = c;
    }
    if (res) {
        if (cur >= size) cur = size - 1;
        res[cur] = '\0';
    }
    return res;
}

char** cmd_get(FILE* f) {
    char *str;
    unsigned int len;

    do {
        if (!(str = get_line(f))) return NULL;
    } while ((len = strlen(str)) == 0);
    return cmd_split(str);
}

void cmd_free(char** cmd) {
    if (cmd) free(cmd[0]);
    free(cmd);
}
