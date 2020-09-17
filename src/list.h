#ifndef LIST_H
#define LIST_H

struct StoneList {
    unsigned int row, col;
    struct StoneList* next;
};

struct StoneList* list_new();
int list_push(struct StoneList** list, unsigned int row, unsigned int col);
int list_pop(struct StoneList** list, unsigned int* row, unsigned int* col);
int list_delete(struct StoneList** list, void* obj);
void list_flush(struct StoneList** list);

#endif
