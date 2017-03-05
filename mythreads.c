#include "queue.h"
#include "mythreads.h"
#include "list.h"
#include "thread.h"
#include "lock.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <ucontext.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <assert.h>
#include <sys/time.h>

lock_t** locks;
ThreadQueue_t* queue;
static unsigned long long id_maker;

static void interruptDisable() {
	assert(!interruptsAreDisabled);
	interruptsAreDisabled = 1;
}

static void interruptEnable() {
	assert(interruptsAreDisabled);
	interruptsAreDisabled = 0;
}

#define T thread_t

void threadInit() {
/*  If this is the first time a thread is being created
 *		we want to initialize the library
 *		which also means assigning the parent thread as a current thread
 */
	locks = malloc( NUM_LOCKS * sizeof(lock_t));
	assert( locks != NULL);
	
	queue = malloc( sizeof(ThreadQueue_t));
	assert( queue != NULL);

	id_maker = 1;

	T* main_data = (T*) malloc(sizeof(T));
	assert( main_data != NULL);

	getcontext(&main_data->context);
	main_data->id = 1;	
	main_data->state = running;
	queue->nthreads = 1;

	queue->main = list_create(main_data);
	queue->head = queue->current = queue->main;
	assert( queue->main != NULL);
}

static void run(T* temp) {

	assert( temp != NULL);
	temp->state = running;

	//interruptEnable();

	temp->results = temp->function( temp->args);

	//interruptDisable();

	temp->state = dead;
	--queue->nthreads;

	threadYield();

}	

T* create_tcb( thFuncPtr funcPtr, void* argPtr) {

 //  malloc the thread struct
	T* temp =(T*) malloc(sizeof(T));
	assert( temp != NULL);
	
 //  set the context to a wrapper function 
 //  which will pass the arguments into the start routine
	getcontext( &temp->context);

 //  Then we push this into the TCB
	temp->context.uc_stack.ss_sp = malloc(STACK_SIZE);
	assert(temp->context.uc_stack.ss_sp != NULL); 
	temp->context.uc_stack.ss_size = STACK_SIZE;
	temp->context.uc_stack.ss_flags = 0;
	// don't use uc_link in project 2

	temp->function =  funcPtr;
	temp->args = argPtr;
	temp->id = ++id_maker;
	
	
	temp->state = running;
	makecontext( &temp->context, (void (*)(void)) run, 1, temp); 

	return temp;

}

int threadCreate( thFuncPtr funcPtr, void* argPtr) {

	interruptDisable();

	T* new_thread = create_tcb(funcPtr, argPtr);

	list_insert_after( queue->current, new_thread);
	assert(queue->current->next != NULL);

	++queue->nthreads;
	threadYield();

	size_t id_val = new_thread->id;

	interruptEnable();

	return id_val;
}

void set_my_state( list_node* temp, State_t my_state) {

	temp->data->state = my_state;
}

/*
 * threadjoin
 * wait until it has finished executing before continuing
 *  It should pass a void** where we can pass back
 *		 what the user thread exited with
 *	When a thread joins on another thread 
 *		we don’t want it to start executing it until the thread it joined on
 *			has finished executing
 *		when a thread blocks
 *			change the status to waiting
 *	add some checks to avoid deadlocks 
 *	if a join happens
 *		we always have at least one thread to execute
 *		and that the thread being joined on exists
 *		and is in the waiting state (or current)
 *	add a little clean up code that gets called by our wrapper function
 *		to ensure we change the status of the joined threads
 *	
 */	
void threadJoin( int thread_id, void **result) {

	interruptDisable();

	list_node* temp = queue->head;
	while( temp->data->id != thread_id) {
		temp = temp->next;
	}

	while( temp->data->state != dead) {
		
		set_my_state( node_self(queue), blocked);
		threadYield();
	} 
	
	*result = temp->data->results;

	interruptEnable();
}

/*
 * When a thread joins
 *		it needs to give up the CPU for the other threads
 *	 yield function just needs to
 *	 push the current thread back into the linked list
 *	 then get the next runnable thread
 *		and perform a context switch
 */
void threadYield() {

	list_node* temp = queue->current;

	do {
		increment_queue( queue);
	
	} while ( queue->current->data->state == dead ); 
	
	swapcontext( &temp->data->context, &queue->current->data->context);
	
}


/* 
 * an existing thread should call the exit function
 * in the user level thread library
 * but we still should handle the case when it doesn’t
 * The exit function needs to accept a void* which enables the user thread
 *		to pass information back to the thread that joined on it
 *	then perform a cleanup routine on joined threads	 
 */
void threadExit( void *result) {

	interruptDisable();

	queue->current->data->results = result;
	queue->current->data->state = dead;

	//list_node* temp = queue->current;

	increment_queue( queue);
		
	//list_remove(&queue->main, temp);

	threadYield();

	interruptEnable();
}



void threadLock(int lockNum) {

	assert(lockNum < NUM_LOCKS);

	while( locks[lockNum]->status == locked) {
	
		set_my_state( node_self(queue), blocked);
		threadYield();
	}
	locks[lockNum]->status = locked;
	locks[lockNum]->thread_id = node_self(queue)->data->id;	
}


 static int threadwait_t = 0;

void threadUnlock( int lockNum) {

	assert(lockNum < NUM_LOCKS);

	if( node_self(queue)->data->id == locks[lockNum]->thread_id || threadwait_t) {

		locks[lockNum]->status = unlocked;
	}
	
}

void threadWait( int lockNum, int conditionNum) {

	assert(lockNum < NUM_LOCKS);
	assert(conditionNum < CONDITIONS_PER_LOCK);

	interruptDisable();

	if( !threadwait_t) {
		++threadwait_t;
	}

	threadUnlock(lockNum);

	if(threadwait_t) {
		--threadwait_t;
	}

	while( !locks[lockNum]->conditions[conditionNum]) {
		set_my_state( node_self(queue), blocked);
		threadYield();
	}	

	interruptEnable();
	
}

void threadSignal( int lockNum, int conditionNum) {

	assert(lockNum < NUM_LOCKS);
	assert(conditionNum < CONDITIONS_PER_LOCK);

	interruptDisable();

	if( !locks[lockNum]->conditions[conditionNum]) {

		++locks[lockNum]->conditions[conditionNum];
	}

	interruptEnable();
}



























