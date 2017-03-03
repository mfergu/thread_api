#include "mythreads.h"
#include "list.h"
#include <ucontext.h>
#include <stdio.h>
#include <string.h>
#include <usr/include/signal.h>
#include <sys/time.h>

int locks[NUM_LOCKS];
int conditions[NUM_LOCKS][CONDITIONS_PER_LOCK];

#define T Thread_T
typedef struct T {
	ucontext_t context;	/* tcb vars */
	thFuncPtr function;
	void* args;
	void* results;
	int id;
	int alerted;
	int done;
	T link;	/* list vars */
	T *inqueue;
	T handle;
	T join;
	T next;
}

static T ready = NULL;
static T current;		/* currently executing thread */
static int nthreads;	/* number of active threads */
static T join;
static T freelist;
static int critical;
static ucontext_t mainContext;	/* main process executing */

/* from header and is extern */
static void interruptsAreDisabled() {
	interruptsAreDisabled = 1;
}

/* from header and is extern */
static void interruptsAreEnabled() {
	interruptsAreDisabled = 0;
}

static void run(void) {

	T thread = current;
	//context switch
	// put current context into ready queue's 1st element from yield?  
	getcontext(ready->context);
	setcontext(&thread.context);
}	

static void testalert(void) {
	if(current->alerted) {
		current->alerted = 0;
		RAISE(Thread_Alerted);
	}
}

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

T Thread_self(void) {
	assert(current);
	return current;
}

void ThreadYield(void) {
	assert(current);
	put(current, &ready);
	run();
}

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













void _ENDMONITOR(void) {}





















