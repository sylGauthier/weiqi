#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "utils.h"

int move_to_str(char dest[3], unsigned char row, unsigned char col) {
    if (row > 25 || col > 25) return 0;
    dest[0] = col + 'A';
    if (dest[0] >= 'I') dest[0]++;
    sprintf(dest + 1, "%d", row + 1);
    return 1;
}

int str_to_move(unsigned char* row, unsigned char* col,
                char* pass, const char* str) {
    if (!strcmp(str, "PASS")) {
        *pass = 1;
        return 1;
    }
    *pass = 0;
    if (       strlen(str) < 2
            || str[0] < 'A' || str[0] > 'Z'
            || str[1] < '0' || str[1] > '9'
            || (strlen(str) == 3 && (str[2] < '0' || str[2] > '9'))
            || strlen(str) > 3) {
        fprintf(stderr, "Error: move must be letter and number, "
                        "like D4, Q16, etc.\n");
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

int read_line(int fd, char* buf, size_t size) {
    unsigned int i = 0;

    do {
        if (read(fd, buf, sizeof(char)) != sizeof(char)) {
            fprintf(stderr, "Error: read error\n");
            return 0;
        }
    } while (*(buf++) != '\n' && ++i < size);
    *(buf - 1) = '\0';
    return 1;
}

int write_str(int fd, const char* s) {
    size_t len, off = 0;
    int w = -1;

    len = strlen(s);
    for (off = 0; off < len; off += w) {
        if ((w = write(fd, s + off, len - off)) == -1)
            break;
    }
    if (w == -1) {
        fprintf(stderr, "Error: write_str: IO error\n");
        return 0;
    }
    return 1;
}
