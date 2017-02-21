
#include "mythreads.h"
#include <ucontext.h>
#include <stdio.h>
#include <string.h>
#include <usr/include/signal.h>
#include <sys/time.h>
#include "assert.h"
#include "mem.h"
#include "sem.h"

/* WORK IN PROGRESS 
 *	anywhere sp is used somehow needs to be replaced with ucontext functions
 */

/* used to delimit the code in the thread library from interrupts*/
void _MONITOR(void) {}

typedef struct Thread_T {

	ucontext_t context;
	Thread_T link;
	Thread_T *inqueue;
	Thread_T handle;
	Except_Frame *estack;
	int code;
	Thread_T join;
	Thread_T next;
	int alerted;
} Thread_T;

static Thread_T ready = NULL;
static Thread_T current;
static int nthreads;
static Thread_T root;
static Thread_T join0;
static Thread_T freelist;
const Except_T Thread_Alerted = { "Thread alerted"};
const Except_T Thread_Failed = { "Thread creation failed"};
static int critical;
/* declaration of _swtch(Thread_T thread, Thread_T to); replaced with ucontext.h functions */

/* METHODS FOR QUEUE */

//adds a thread to the queue
static void put( Thread_T thread, Thread_T *queue) {

	assert( thread);
	assert( thread->inqueue == NULL && thread->link == NULL);
	
	if( *queue) {
		thread->link = ( *queue)->link;
		( *queue)->link = thread;
	}
	else {
		thread->link = thread;
	}
	*queue = thread;
	thread->inqueue = q;
}

//removes the first element from the queue
static Thread_T get( Thread_T *queue) {

	Thread_T thread;
	
	assert( !isempty(queue));

	thread = (*queue)->link;
	if(thread == *queue) {
		*queue = NULL;
	}
	else {
		(*queue)->link = thread->link;
	}	
	
	assert(thread->inqueue == queue);
	
	thread->link = NULL;
	thread->inqueue = NULL;
	return thread;
}
	
//removes a queued thread from the queue
static void delete( Thread_T thread, Thread_T *queue) {

	Thread_T pull;
	
	assert( thread->link && thread->inqueue == queue);
	assert( !isempty(*queue));

	for(pull = *queue; pull->link != thread; pull = pull->link)
		;
	if( pull == thread) {
		*queue = NULL;
	}
	else {
		pull->link = thread->link;
		if( *queue == thread) {
			*queue = pull;
		}
	}

	thread->link = NULL;
	thread->inqueue = NULL;
}

/* NOW FOR SOMETHING COMPLETELY DIFFERENT
 * 
 * Thread Management API */

	/* a call to a thread library function should not be interrupted 
	 */
static int interrupt( int sig, struct sigcontext sc) {
	/* catch any interrupts between library functions
	 *  or if critical is greater than zero and ignore them */
	if( critical || sc.rip >= (unsigned long)_MONITOR 
		&& sc.rip <= (unsigned long)_ENDMONITOR)
		return 0;
	
	put( current, &ready);

	do {
		critical++;
		sigsetmask( sc.oldmask);
		critical--;
	} while(0);
	run();
	return 0;
}

/*switch from the currently running thread to the one at the front of the queue
 */
static void run(void) {
	
	Thread_T thread = current;
	current = get(&ready);
	thread->estack = Except_stack;
	Except_stack = current->estack;
	//replace _swtch(thread, current) with ucontext.h library functions
}
	
/* deallocate the current thread's stack 
 * because a thread can't deallocate its own stack while it is using it */
static void release(void) {
	
	Thread_T thread;
	
	do{
		critical++;
		while (( thread = freelist) != NULL ) {
			freelist = thread->next;
			FREE(thread);
		}
		critical--;
	} while (0);
}

int Thread_init(void) {

	assert( preempt == 0 || preempt == 1);
	assert( current == NULL);
	root.handle = &root;
	nthreads = 1;
	
	if( preempt) {
		
		struct sigaction sa;
		memset( &sa, '\0', sizeof( sa));
		sa.sa_handler = ( void (*)())interrupt;
		if( sigaction( SIGVTALRM, &sa, NULL) < 0) {
			return 0;
		}
		
		struct itimerval it;
		it.it_value.tv_sec = 0;
		it.it_value.tv_usec = 10;
		it.it_interval.tv_sec = 0;
		it.it_interval.tv_usec = 10;

		if( setitimer( ITIMER_VIRTUAL, &it, NULL) < 0) {
			return 0;
		}
	}
	return 1;
}	

Thread_T Thread_self(void) {
	assert( current);
	return current;
}

void Thread_pause(void) {
	assert(current);
	put(current, &ready);
	run();
}

/* thread join suspends the calling thread until the new thread terminates. When the new thread termi-
 * nates, Thread_join returns new thread’s exit code. If t=null, the calling thread
 * waits for all other threads to terminate, and then returns zero. It is a
 * c.r.e. for t to name the calling thread or for more than one thread to
 * pass a null t.
 */
int Thread_join( Thread_T thread) {
	assert( current && thread != current);
	testalert();
	if ( thread) {
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
		assert( isempty( join0));
		if( nthreads > 1) {
			put(current, &join0);
			run();
			testalert();
		}
		return 0;
	}	
}

/* thread_exit terminates the calling thread and passes code to any threads waiting
 * for the calling thread to terminate 
 */
void Thread_exit( int code) {
	assert( current);
	release();
	if( current != &root) {
		current->next = freelist;
		freelist = current;
	}
	current->handle = NULL;
	while( !isempty(current->join)) {
		Thread_T thread = get( &current->join);
		thread->code = code;
		put( thread, &ready);
	}
	if ( !isempty( Join0) && nthreads == 2) {
		assert(isempty(ready));
		put( get( &join0), &ready);
	}
	if( --nthreads == 0) {
		exit(code);
	}
	else {
		run();
	}
}

void Thread_alert( Thread_T thread) {
	assert(current);
	assert(thread && thread->handle == thread);
	
	thread->alerted = 1;
	
	if( thread->inqueue) {
		delete( thread, thread->inqueue);
		put( thread, &ready);
	}
}

Thread_T Thread_new( int apply(void*), void* args, int nbytes, ...) {

	Thread_T thread;
	assert(current);
	assert(apply);
	assert(args && nbytes >= 0 || args == NULL);
	
	if( args == NULL) {
		nbytes = 0;
	}
	
	do{ 
		critical++;
		TRY
			thread = ALLOC(STACK_SIZE);
			memset(thread, '\0', sizeof(*t));
		EXCEPT(Mem_Failed)
			thread = NULL;
		END_TRY;
		critical--;
	} while(0);
	
	if( thread == NULL) {
		RAISE(Thread_Failed);
	}
	//next line probably needs to be replaced with a function from ucontext.h library	
	thread->sp = (void*)(((unsigned long)t + stacksize)&~15U);
	
	thread->handle = thread;
	if( nbytes > 0) {
	//next line probably needs to be replaced with a function from ucontext.h library	
		thread->sp -= ((nbytes + 15U)&~15)/sizeof(*thread->sp);
		do {
			critical++;
			memcpy(thread->sp, args, nbytes);
			critical--;
		} while(0);	
		
		args = thread->sp;
	}
	
	//next lines probably needs to be replaced with a function from ucontext.h library	
    { extern void _thrstart(void);                                                                            
      t->sp -= 16/4;    /* keep stack aligned to 16-byte boundaries */                                        
      *t->sp = (unsigned long)_thrstart;                                                                      
      t->sp -= 16/4;                                                                                          
      t->sp[4/4]  = (unsigned long)apply;                                                                     
      t->sp[8/4]  = (unsigned long)args;                                                                      
      t->sp[12/4] = (unsigned long)t->sp + (4+16)/4; } 

	nthreads++;
	put(thread, &ready);
	return thread;
}

Sem_T* Sem_new(int count) {
	Sem_T *sem;
	NEW(sem);
	Sem_init(sem, count);
	return sem;
}

void Sem_init( Sem_T *sem, int count) {
	assert(current);
	assert(sem);
	sem->count = count;
	sem->queue = NULL;
}

void Sem_wait( Sem_T* sem) {
	assert( curent);
	assert( sem);
	testalert();
	if ( sem->count <= 0 ) {
		put(current, (Thread_T*)&s->queue);
		run();
		testalert();
	}
	else {
		--sem->count;
	}
}

void Sem_signal( Sem_T* sem) {
	assert(current);
	assert(s);
	if( sem->count == 0 && !isempty( sem->queue)) {
		Thread_T thread = get((Thread_T*)&sem->queue);
		assert(!thread->alerted);
		put(thread, &ready);
	}
	else {
		++sem->count;
}

/* used to delimit the code in the thread library from interrupts */
void _ENDMONITOR(void) {};














