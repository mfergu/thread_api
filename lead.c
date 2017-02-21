// context switching info
// http://pubs.opengroup.org/onlinepubs/7908799/xsh/ucontext.h.html
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include </usr/include/signal.h>
#include <sys/time.h>
#include "assert.h"
#include "mem.h"
#include "sem.h"
#include "mythreads.h"
#include "list.h"

/*thread.c begins with the
 * vacuous function _MONITOR, and the assembly-language code in swtch.s
 * ends with a definition for the global symbol _ENDMONITOR.
 *
 * If the object files are loaded into the program so that
 * the object code for swtch.s follows the object code for thread.c,
 *	 then the interrupted thread is executing a Thread or Sem function
 *		 if its location counter is between _MONITOR and _ENDMONITOR.
 *			Thus, if critical is nonzero, or scp->sc_pc is between _MONITOR and _ENDMONITOR,
 *			interrupt returns and thus ignores this timer interrupt.
 *		 Otherwise, interrupt puts the current thread
 * on ready and runs another one. */
void _MONITOR(void) {}
extern void _ENDMONITOR(void);
#define T thread_t
#define isempty(q) ((q) == NULL)

// thread control block
struct T {

	unsigned long *sp; //holds OS values
	T link; //link hold the next tcb in the queue
	T *inqueue; //points to the queue 
	T handle; //
	Except_Frame *estack;
	int code;
	T join;
	T next;
	int alerted;
};

static int preempt = 1;
static T ready = NULL;
static T current = NULL; //current thread that holds the processor
static int nthreads;
static struct thread_t root;
static T join0;
static T freelist;
const Except_T Thread_Alerted = { "Thread alerted" };
const Except_T Thread_Failed = { "Thread creation failed" };
static int critical;
extern void _swtch( T from, T to);


//ucontext switch function need to put here

//put appends a thread to an empty or nonempty queue
static void put( T thread, T *queue) {
	
	assert(thread);
	assert(thread->inqueue == NULL && thread->link == NULL);
	if(*q) {
	
		thread->link = (*queue)->link;
		(*queue)->link = thread;
	}
	else {
		thread->link = thread;
	}
	
	*queue = thread;
	thread->inqueue = queue;
}
		
//get removes the first element of a given queue
static T get( T *queue) {
	
	T thread;
	assert( !isempty(*queue));
	thread = (*queue)->link;
	if (thread == *queue)
        *queue = NULL;
    else
        (*queue)->link = thread->link;
    assert(thread->inqueue == q);
    thread->link = NULL;
    thread->inqueue = NULL;
    return thread;
}    

// switches from the current running thread to the thread at the head of the ready queue
static void run(void) {

	T thread = current;
	current = get(&ready);
	thread->estack = Except_stack;
	Except_stack = current->estack;
	//input a context switch
	_swtch(t, current);
}

// marks a thread as alerted by setting a flag on its TCB
// and removes it from the queue (if it's in one) 
static void testalert(void) {

	if (current->alerted) {

		current->alerted = 0;
		RAISE(Thread_Alerted);
	}
}

static void release(void) {

	T thread;

	do { critical++;

	while ( ( thread = freelist) != NULL) {

		freelist = thread->next;
		FREE(thread);
	}

	critical--; } while (0);
}

static int interrupt(int sig, struct sigcontext sc) {

	if (critical ||
		sc.rip >= (unsigned long)_MONITOR
	&& sc.rip <= (unsigned long)_ENDMONITOR) {

		return 0;
	}
	put(current, &ready);
	do { critical++;
	sigsetmask(sc.oldmask);
	critical--; } while (0);
	run();
	return 0;
}

// creates the root thread and initializes for preemptive scheduling
int Thread_init(void) {

	assert(current == NULL);
	root.handle = &root;
	current = &root;
	nthreads = 1;
	if (preempt) {
		{
			struct sigaction sa; //sa has 3 fields 
			memset(&sa, '\0', sizeof sa);

			//sa_handler holds the address of the function to call
			// when the SIGVTALRM signal occurs
			sa.sa_handler = (void (*)())interrupt;

			// specifies the action to be associated with SIGVTALRM
			if ( sigaction(SIGVTALRM, &sa, NULL) < 0) {
	
				return 0;
			}
        }
        {
			//used to specify when a timer should expire
			struct itimerval it;

			// it_value is the period between now and the first timer interrupt
			it.it_value.tv_sec     =  0;
			it.it_value.tv_usec    = 10;

			// it_interval is the period between successive timer interrupts
			it.it_interval.tv_sec  =  0;
			it.it_interval.tv_usec = 10;
			if (setitimer(ITIMER_VIRTUAL, &it, NULL) < 0) {

				return 0;
			}
		}
	}
	return 1;
}

// test
T Thread_self(void) {
	assert(current);
	return current;
}

//test
void Thread_pause(void) {
	assert(current);
	put(current, &ready);
	run();
}

int Thread_join(T thread) {

	assert(current && thread != current);
	testalert();

	// wait for thread to terminate
	if (thread) {

		if (thread->handle == thread) {

			put(current, &thread->join);
			run();
			testalert();
			return current->code;
		}
		else {
			return -1;
		}
	}

	// wait for all threads to terminate
	else {

		assert(isempty(join0));
		if (nthreads > 1) {
			put(current, &join0);
			run();
			testalert();
		}
		return 0;
	}
}

void Thread_exit(int code) {
	assert(current);
	release();
	if (current != &root) {
		current->next = freelist;
		freelist = current;
	}
	current->handle = NULL;
	while (!isempty(current->join)) {
		T thread = get(&current->join);
		thread->code = code;
		put(thread, &ready);
	}
	if (!isempty(join0) && nthreads == 2) {
		assert(isempty(ready));
		put(get(&join0), &ready);
	}
	if (--nthreads == 0)
		exit(code);
	else
		run();
}

void Thread_alert(T thread) {
	assert(current);
	assert(thread && thread->handle == thread);
	thread->alerted = 1;
	if (thread->inqueue) {
		delete(thread, thread->inqueue);
		put(thread, &ready);
	}
}

T Thread_new(int apply(void *), void *args, int nbytes, ...) {
	T thread;
	assert(current);
	assert(apply);
	assert(args && nbytes >= 0 || args == NULL);
	if (args == NULL)
		nbytes = 0;
	{
		int stacksize = STACK_SIZE;
		release();
		do { critical++;
		TRY
			thread = ALLOC(stacksize);
			memset(thread, '\0', sizeof *thread);
		EXCEPT(Mem_Failed)
			thread = NULL;
		END_TRY;
		critical--; } while (0);
		if (thread == NULL)
			RAISE(Thread_Failed);
		thread->sp = (void *)(((unsigned long)t + stacksize)&~15U);
	}
	thread->handle = thread;
	if (nbytes > 0) {
		thread->sp -= ((nbytes + 15U)&~15)/sizeof (*thread->sp);
		do { critical++;
		memcpy(thread->sp, args, nbytes);
		critical--; } while (0);
		args = thread->sp;
	}
	 
	//insert context switch here
	extern void _thrstart(void);
	thread->sp -= 16/4;    /* keep stack aligned to -byte boundaries */
	*thread->sp = (unsigned long)_thrstart;
	thread->sp -= 16/4;
	thread->sp[4/4]  = (unsigned long)apply;
	thread->sp[8/4]  = (unsigned long)args;
	thread->sp[12/4] = (unsigned long)t->sp + (4+16)/4; 

	nthreads++;
	put(thread, &ready);
	return thread;
}

#undef T
#define T Sem_T

T *Sem_new(int count) {

	T *s;
	NEW(s);
	Sem_init(s, count);
	return s;
}

void Sem_init(T *s, int count) {

	assert(current);
	assert(s);
	s->count = count;
	s->queue = NULL;
}

void Sem_wait(T *s) {

	assert(current);
	assert(s);
	testalert();
	if (s->count <= 0) {
		put(current, (Thread_T *)&s->queue);
		run();
		testalert();
	} else
		--s->count;
}                                                                                                             

void Sem_signal(T *s) {

	assert(current);
	assert(s);
	if (s->count == 0 && !isempty(s->queue)) {
		Thread_T t = get((Thread_T *)&s->queue);
		assert(!t->alerted);
		put(t, &ready);
	} else
		++s->count;
}
#undef T

extern int threadCreate(thFuncPtr funcPtr, void *argPtr) {

	//add new function to the end of the thread list
	node_t* temp = list_find_numeric(threadList, activeThreads);
	getcontext( &temp->data->context );

	temp->data->context.uc_link = 0;
	temp->data->stack = malloc( STACK_SIZE);	
	temp->data->context.uc_stack.ss_sp = temp->data->stack;	
	temp->data->context.uc_stack.ss_size = STACK_SIZE;
	temp->data->context.uc_stack.ss_flags = 0;

	if( temp->data->stack == 0) {
		
		return;
	}

	//create context and initialize 
	makecontext( &temp->data->context, (void (*)(void)) &threadStart, 1, func);
	++numFibers;

	return 1;
}

static void threadStart( void (*func)(void)) {

	node_t* temp = list_find_numeric(threadList, currentThread);
	temp->data->active = 1;
	func();
	temp->data->active = 0;
	threadYield();
}
	
extern void threadYield(void) {

	if( inThread) {	
		
		//find the context to switch to in the list
		node_t* temp = list_find_numeric(threadList, currentThread);
		
		getcontext( temp->data->context, &mainContext);

	}
	else {
		
		if( activeThreads == 0 ) {
			
			return;
		}
		
		currentThread = currentThread + 1;
		
		inThread = 1;
		
		//find the context to switch to in the list
		node_t* temp = list_find_numeric(threadList, currentThread);
		
		swapcontext( &mainContext, temp->context);

		inThread = 0;

		if( threadList[currentThread].active == 0 ) {
			
			free( threadList[currentThread].stack);

			--activeThreads;

			if( currentThread != activeThreads ) {
				
				node_t* tempA = list_find_numeric(threadList, currentThread); 	
				node_t* tempB = list_find_numeric(threadList, activeThread); 	
				threadList[ currentThread] = threadList[ activeThreads];
			}
		
			threadList[ activeThreads].active = 0;
		}
	}
	return;
}

//function pointer for list_find to find the thread number in the threadList
//given a number and a queue list find

extern void threadJoin(int thread_id, void **result);


//exits the current thread -- closing the main thread, will terminate the program
extern void threadExit(void *result); 

extern void threadLock(int lockNum); 
extern void threadUnlock(int lockNum); 
extern void threadWait(int lockNum, int conditionNum); 
extern void threadSignal(int lockNum, int conditionNum); 

//this 
extern int interruptsAreDisabled;
