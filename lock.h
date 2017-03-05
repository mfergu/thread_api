
#pragma once
#include "mythreads.h"

typedef enum status { locked, unlocked} status_t;

typedef struct lock_t {
	size_t thread_id;
	int* conditions;
	status_t status;
} lock_t;

