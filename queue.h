
#pragma once
#include "list.h"
typedef struct queue{
    list_node *main, *head, *current;
    size_t nthreads;
} ThreadQueue_t;

void increment_queue(ThreadQueue_t*);
list_node* node_self(ThreadQueue_t*);
