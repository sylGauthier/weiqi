#include <stdlib.h>

#include "list.h"

struct StoneList* list_new() {
    struct StoneList* ret;
    if ((ret = malloc(sizeof(*ret)))) {
        ret->col = 0;
        ret->row = 0;
        ret->next = NULL;
    }
    return ret;
}

int list_push(struct StoneList** list, unsigned char row, unsigned char col) {
    struct StoneList* newHead;
    if ((newHead = list_new())) {
        newHead->row = row;
        newHead->col = col;
        newHead->next = *list;
        *list = newHead;
        return 1;
    }
    return 0;
}

int list_pop(struct StoneList** list, unsigned char* row, unsigned char* col) {
    struct StoneList* tmp;
    if (*list) {
        tmp = *list;
        *row = tmp->row;
        *col = tmp->col;
        *list = tmp->next;
        free(tmp);
        return 1;
    }
    return 0;
}

void list_flush(struct StoneList** list) {
    unsigned char row, col;

    while (*list) {
        list_pop(list, &row, &col);
    }
}
