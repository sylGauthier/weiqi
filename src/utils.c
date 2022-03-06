#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "utils.h"

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
