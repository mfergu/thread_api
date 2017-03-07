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
ThreadQueue_t* AllQueue, *WaitQueue;

static unsigned long long id_maker;
int interruptsAreDisabled = 0;

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
	
	AllQueue = malloc( sizeof(ThreadQueue_t*));
	assert( AllQueue != NULL);

	id_maker = 1;

	T* main_data = (T*) malloc(sizeof(T));
	assert( main_data != NULL);

	getcontext(&main_data->context);
	main_data->id = 1;	
	main_data->state = running;
	AllQueue->nthreads = 1;

	AllQueue->main = list_create(main_data);
	AllQueue->head = AllQueue->current = AllQueue->main;
	assert( AllQueue->main != NULL);
}

static void run(T* temp) {

	assert( temp != NULL);
	temp->state = running;

	interruptEnable();

	temp->results = temp->function( temp->args);

	interruptDisable();

	temp->state = dead;
	--AllQueue->nthreads;

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

	list_insert_after( AllQueue->current, new_thread);
	assert(AllQueue->current->next != NULL);

	++AllQueue->nthreads;
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

	//current thread is waiting on thread_id to exit
	AllQueue->current->data->waiting_on = thread_id;
	
	//if wait queue not ready
	if( WaitQueue == NULL) {
		WaitQueue = malloc(sizeof(ThreadQueue_t*));
		assert(WaitQueue != NULL);
		//add current thread to wait queue
		WaitQueue->main = list_create(AllQueue->current->data);
		assert(WaitQueue->main != NULL);
		WaitQueue->current = WaitQueue->head = WaitQueue->main;
	}
	
	//add current thread to wait queue
	list_insert_end( WaitQueue->head, AllQueue->current->data); 

	list_node* temp = AllQueue->head;
	while( temp->data->id != thread_id) {
		temp = temp->next;
	}

	while( temp->data->state != dead) {
		
		set_my_state( node_self(AllQueue), blocked);
		threadYield();
	} 
	
	if( result) {
		*result = temp->data->results;
	}

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

	list_node* temp = AllQueue->current;

	do {
		increment_queue( AllQueue);
	
	} while ( AllQueue->current->data->state == dead ); 
	
	swapcontext( &temp->data->context, &AllQueue->current->data->context);
	
}

int thread_is_waiting( int completed_thread) {
		
	list_node* temp = WaitQueue->main;
	while(temp->data->waiting_on != completed_thread || temp->next != NULL) {
		temp = temp->next;
	}
	return temp->data->waiting_on == completed_thread? 1 : 0;
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

	if(result) {
		AllQueue->current->data->results = result;
	}
	AllQueue->current->data->state = dead;

	//if the waiting queue has a thread waiting on the current exit thread
	//if(thread_is_waiting)
	//list_node* temp = AllQueue->current;

	increment_queue( AllQueue);
		
	//list_remove(&AllQueue->main, temp);

	threadYield();

	interruptEnable();
}



void threadLock(int lockNum) {

	assert(lockNum < NUM_LOCKS);

	while( locks[lockNum]->status == locked) {
	
		set_my_state( node_self(AllQueue), blocked);
		threadYield();
	}
	locks[lockNum]->status = locked;
	locks[lockNum]->thread_id = node_self(AllQueue)->data->id;	
}


 static int threadwait_t = 0;

void threadUnlock( int lockNum) {

	assert(lockNum < NUM_LOCKS);

	if( node_self(AllQueue)->data->id == locks[lockNum]->thread_id || threadwait_t) {

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
		set_my_state( node_self(AllQueue), blocked);
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



























