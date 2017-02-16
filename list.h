// modified from https://github.com/dxtr/list 
#pragma once 

//data structure to hold thread information
typedef struct {                                                                                              
                                                                                                              
    ucontext_t context; //stores current context                                           
    uint8_t active; //active flag                                                      
    void* stack; //                                                                                              
} thread_t;  


//data structure to represent queue
typedef struct node_t {

	struct thread_t *data;
	struct node_t *next;
} node_t;

//linked list
node_t* list_find_numeric(node_t** head, size_t numberInQueue);
node_t* list_create(thread_t *data);
thread_t list_destroy(node_t **list);
node_t* list_insert_after(node_t *node, thread_t *data);
node_t* list_insert_beginning(node_t *list, thread_t *data);
node_t* list_insert_end(node_t *list, thread_t *data);
thread_t list_remove(node_t **list, node_t *node);
thread_t list_remove_by_data(node_t **list, thread_t *data);
node_t* list_find_node(node_t *list, node_t *node);
node_t* list_find_by_data(node_t *list, thread_t *data);
node_t* list_find(node_t *list, int(*func)(node_t*,thread_t*), thread_t *data);


