#include "queue.h"

void increment_queue(ThreadQueue_t* queue) {

	if(queue->current->next == NULL) {
		queue->current = queue->main;
	}
	else {
		queue->current = queue->current->next;
	}
}

list_node* node_self(ThreadQueue_t* queue) {
	return queue->current;
}
