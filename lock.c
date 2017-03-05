#include "lock.h"
#include <stdlib.h>

void initialize_locks( lock_t** locks) {

	int i = 0;
	while( i < NUM_LOCKS) {
		(*locks+i)->thread_id = 0;
		(*locks+i)->conditions = malloc(CONDITIONS_PER_LOCK * sizeof(int));
		(*locks+i)->status = unlocked;
	}
}
