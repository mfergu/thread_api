#include "mythreads.h"
#include "list.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <ucontext.h>
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
typedef struct T {
	ucontext_t context;	/* tcb vars */
	thFuncPtr function;
	void* args;
	void* results;
	size_t id;
	int active;
	int blocked;
	int complete;
}T;

static list_node* front, * current;
static int nthreads;	/* number of active threads */

static T* join;
static T freelist;
static int critical;
static T* ready = NULL;

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

	T* main = (T*) malloc(sizeof(T));
	assert( main != NULL);

	assert(getcontext(&main->context) != -1);
	main->id = 1;	
	main->active = 1;
	main->blocked = 0;
	main->complete = 0;
	nthreads = 1;

	threadQueue = list_create(main);
	assert( threadQueue != NULL);

}

static void run(T* temp) {

	//needs review for yielding and variable semantics
	temp->active = 1;
	temp->results = temp->function( temp->args);
	temp->complete = 1;
	temp->active = 0;
	threadYield();
}	

extern int threadCreate( thFuncPtr funcPtr, void* argPtr) {

	interruptDisable();

	T* temp =(T*) malloc(sizeof(T));
	assert( temp != NULL);

	assert( getcontext( &temp->context) != -1);	
	temp->context.uc_stack.ss_sp = malloc(STACK_SIZE);
	assert(temp->context.uc_stack.ss_sp != NULL); 
	temp->context.uc_stack.ss_size = STACK_SIZE;
	temp->context.uc_stack.ss_flags = 0;
	// don't use uc_link in project 2
	
	temp->function =  funcPtr;
	temp->args = argPtr;
	temp->id = (size_t) gettid();
	temp->complete = 0;
	makecontext( &temp->context, (void (*)(void)) run, 1, temp); 

	list_insert_end( threadQueue, temp);

	nthreads++;
	
	interruptEnable();

}

/*
 * void** where we can pass back what the user thread exited with
 *
 * When a thread joins on another thread we don’t want it to start executing it
 *	 until the thread it joined on has finished
 *
 *
extern void threadJoin( int thread_id, void **result) {

	interruptDisable();
	
	list_node temp = ;
}

extern void threadYield(void) {

	interruptDisable();
	
	//if in a thread switch to main	
	
	//if in main switch to a thread

	interruptEnable();

}

/*
static void testalert(void) {
	if(current->alerted) {
		current->alerted = 0;
		RAISE(Thread_Alerted);
	}
}
*/

/*
static void release(void) {
	T thread;
	do {
		critical++;
		while( (thread = freelist) != NULL) {
			freelist = thread->next;
			FREE(thread);
		}
		critical--;
	} while(0);
}
*/

/*
static int interrupt( int sig, struct sigcontext sc) {
	if( critical || sc.rip >= (unsigned long)_MONITOR &&
		sc.rip <= (unsigned long)_ENDMONITOR) {
		return 0;
	}

	put(current, &ready);
	do{
		critical++;
		sigsetmask(sc.oldmask);
		critical--;
	} while(0);
	run();
	return 0;
}
*/

/*
int ThreadInit(void) {
	assert( preempt == 1 || preempt == 0);
	assert( current == NULL);	
	root.handle = &root;
	current = &root;
	nthreads = 1;
	if( preempt) {
		{
			struct sigaction sa;
			memset( &sa, '\0', sizeof(sa));
			sa.sa_handler = (void(*))interrupt;
			if( sigaction(SIGVTALRM, &sa, NULL) < 0) {
				return 0;
			}
		}
		{
			struct itimerval it;
			it.it_value.tv_sec = 0;
			it.it_value.tv_usec = 10;
			it.it_interval.tv_sec = 0;
			it.it_interval.tv_usec = 10;
			if( setitimer(ITIMER_VIRTUAL, &it, NULL) < 0) {
				return 0;
			}
		}
	}
	return 1;
}	
*/

/*
T Thread_self(void) {
	assert(current);
	return current;
}
*/

/*
void ThreadYield(void) {
	assert(current);
	put(current, &ready);
	run();
}
*/

/*
int ThreadJoin( T thread) {
	assert( current && thread != current);
	testalert();
	if( thread) {
		if( thread->handle == thread) {
			put( current, &thread->join);
			run();
			testalert();
			return current->code;
		}
		else {
			return -1;
		}
	}
	else {
		assert( isempty(join));
		if( nthreads > 1) {
			put( current, &join);
			run();
			testalert();
		}
		return 0;
	}
}
*/

/*
void ThreadExit( int code) {
	assert( current);
	release();
	if( current != &root) {
		current->next = freelist;
		freelist = current;
	}
	current->handle = NULL;
	while( !isempty( current->join)) {
		T thread = get( &current->join);
		thread->code = code;
		put( thread, &ready);
	}
	if( !isempty( join) && nthreads == 2) {
		assert( isempty(ready));
		put( get( &join), &ready);
	}
	if( --nthreads == 0) {
		exit( code);
	}
	else {
		run();
	}	
}

*/













