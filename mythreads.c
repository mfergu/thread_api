#include "queue.h"
#include "mythreads.h"
#include "list.h"
#include "thread.h"
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

#ifndef gettid
#define gettid() syscall(SYS_gettid)
#endif

int locks[NUM_LOCKS];
int conditions[NUM_LOCKS][CONDITIONS_PER_LOCK];

#define T thread_t

ThreadQueue_t* queue;
static int in_thread;

/* from header and is extern */
static void interruptDisable() {
	assert(!interruptsAreDisabled);
	interruptsAreDisabled = 1;
}

/* from header and is extern */
static void interruptEnable() {
	assert(interruptsAreDisabled);
	interruptsAreDisabled = 0;
}

void threadInit(void) {
/*  If this is the first time a thread is being created
 *		we want to initialize the library
 *		which also means assigning the parent thread as a current thread
 */

	queue = malloc( sizeof(ThreadQueue_t));
	assert( queue != NULL);

	T* main_data = (T*) malloc(sizeof(T));
	assert( main_data != NULL);

	assert(getcontext(&main_data->context) != -1);
	main_data->id = 1;	
	main_data->state = running;
	queue->nthreads = 1;

	queue->main = list_create(main_data);
	queue->head = queue->current = queue->main;
	assert( queue->main != NULL);
}

static void run(T* temp) {

	assert( temp != NULL);
	//needs review for yielding and variable semantics
	temp->state = running;
	temp->results = temp->function( temp->args);
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
	assert( getcontext( &temp->context) != -1);	

 //  Then we push this into the TCB
	temp->context.uc_stack.ss_sp = malloc(STACK_SIZE);
	assert(temp->context.uc_stack.ss_sp != NULL); 
	temp->context.uc_stack.ss_size = STACK_SIZE;
	temp->context.uc_stack.ss_flags = 0;
	// don't use uc_link in project 2

	temp->function =  funcPtr;
	temp->args = argPtr;
	temp->id = (size_t) gettid();
	temp->state = running ;
	makecontext( &temp->context, (void (*)(void)) run, 1, temp); 

	return temp;

}

/* descrepancy for threadCreate return
 *   needs to return thread id to parent 
 *   also needs to 
 *	if all of the above is successful
 *		return 0 otherwise we want to return a nonzero value
 */
extern int threadCreate( thFuncPtr funcPtr, void* argPtr) {

	interruptDisable();

	T* new_thread = create_tcb(funcPtr, argPtr);
	assert( list_insert_after( queue->current, new_thread) != NULL);

	queue->nthreads++;
	threadYield();

/*	We want the parent thread to know the id of the thread it created,
 *		so we pass it back through the pthread_t* thread argument
 */
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
extern void threadJoin( int thread_id, void **result) {

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
extern void threadYield(void) {

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
extern void threadExit( void *result) {

	interruptDisable();

	queue->current->data->results = result;
	queue->current->data->state = dead;

	list_node* temp = queue->current;

	increment_queue( queue);
		
	list_remove(&queue->main, temp);

	threadYield();

	interruptEnable();
}



extern void threadLock(int lockNum) {

}

extern void threadUnlock( int lockNum) {

}

extern void threadWait( int lockNum, int conditionNum) {

}

extern void threadSignal( int lockNum, int conditionNum) {

}



























