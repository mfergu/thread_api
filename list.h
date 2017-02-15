// pulled from https://github.com/dxtr/list 
#pragma once 

typedef struct node_t {
	void *data;
	struct node_t *next;
} node_t;

/* linked list */
node_t* list_create(void *data);
void list_destroy(node_t **list);
node_t* list_insert_after(node_t *node, void *data);
node_t* list_insert_beginning(node_t *list, void *data);
node_t* list_insert_end(node_t *list, void *data);
void list_remove(node_t **list, node_t *node);
void list_remove_by_data(node_t **list, void *data);
node_t* list_find_node(node_t *list, node_t *node);
node_t* list_find_by_data(node_t *list, void *data);
node_t* list_find(node_t *list, int(*func)(node_t*,void*), void *data);


