// pulled from https://github.com/dxtr/list
#include <stdlib.h>
#include <string.h>
#include "list.h"

/* Creates a list (node) and returns it
 * Arguments: The data the list will contain or NULL to create an empty
 * list/node
 */
node_t* list_create(void *data)
{
	node_t *l = malloc(sizeof(node_t));
	if (l != NULL) {
		l->next = NULL;
		l->data = data;
	}

	return l;
}

/* Completely destroys a list
 * Arguments: A pointer to a pointer to a list
 */
void list_destroy(node_t **list)
{
	if (list == NULL) return;
	while (*list != NULL) {
		list_remove(list, *list);
	}
}

/* Creates a list node and inserts it after the specified node
 * Arguments: A node to insert after and the data the new node will contain
 */
node_t* list_insert_after(node_t *node, void *data)
{
	node_t *new_node = list_create(data);
	if (new_node) {
		new_node->next = node->next;
		node->next = new_node;
	}
	return new_node;
}

/* Creates a new list node and inserts it in the beginning of the list
 * Arguments: The list the node will be inserted to and the data the node will
 * contain
 */
node_t* list_insert_beginning(node_t *list, void *data)
{
	node_t *new_node = list_create(data);
	if (new_node != NULL) { new_node->next = list; }
	return new_node;
}

/* Creates a new list node and inserts it at the end of the list
 * Arguments: The list the node will be inserted to and the data the node will
 * contain
 */
node_t* list_insert_end(node_t *list, void *data)
{
	node_t *new_node = list_create(data);
	if (new_node != NULL) {
		for(node_t *it = list; it != NULL; it = it->next) {
			if (it->next == NULL) {
				it->next = new_node;
				break;
			}
		}
	}
	return new_node;
}

/* Removes a node from the list
 * Arguments: The list and the node that will be removed
 */
void list_remove(node_t **list, node_t *node)
{
	node_t *tmp = NULL;
	if (list == NULL || *list == NULL || node == NULL) return;

	if (*list == node) {
		*list = (*list)->next;
		free(node);
		node = NULL;
	} else {
		tmp = *list;
		while (tmp->next && tmp->next != node) tmp = tmp->next;
		if (tmp->next) {
			tmp->next = node->next;
			free(node);
			node = NULL;
		}
	}
}

/* Removes an element from a list by comparing the data pointers
 * Arguments: A pointer to a pointer to a list and the pointer to the data
 */
void list_remove_by_data(node_t **list, void *data)
{
	if (list == NULL || *list == NULL || data == NULL) return;
	list_remove(list, list_find_by_data(*list, data));
}

/* Find an element in a list by the pointer to the element
 * Arguments: A pointer to a list and a pointer to the node/element
 */
node_t* list_find_node(node_t *list, node_t *node)
{
	while (list) {
		if (list == node) break;
		list = list->next;
	}
	return list;
}

/* Finds an elemt in a list by the data pointer
 * Arguments: A pointer to a list and a pointer to the data
 */
node_t* list_find_by_data(node_t *list, void *data)
{
	while (list) {
		if (list->data == data) break;
		list = list->next;
	}
	return list;
}

/* Finds an element in the list by using the comparison function
 * Arguments: A pointer to a list, the comparison function and a pointer to the
 * data
 */
node_t* list_find(node_t *list, int(*func)(node_t*,void*), void *data)
{
	if (!func) return NULL;
	while(list) {
		if (func(list, data)) break;
		list = list->next;
	}
	return list;
}

