#include "mythreads.h"
#include "list.h"

// data structure for each user thread
typedef struct {
	
	ucontext_t context;
	uint8_t active;
	void* stack;

} thread_t;

//holds each thread in queue
static thread_t threadList[MAX_THREADS];

//number of active threads
static uint8_t activeThreads;

//index of current thread
static uint8_t currentThread;

//boolean if in main(1) or in thread(0)
static uint8_t inThread;

static ucontext_t mainContext;

//holds each thread in queue
extern void threadInit() {

	activeThreads = 0;
	currentThread = -1;
	for(int i = 0; i < MAX_THREADS; i++) {
		threadList[i].active = 0;
	}
}

extern int threadCreate(thFuncPtr funcPtr, void *argPtr) {

	
}
extern void threadYield(void) {
	if( inThread) {	

		getcontext( &threadList[currentThread].context, &ogContext);
	}
	else {
		
		if( activeThreads == 0 ) {
			
			return;
		}
		
		currentThread = ( currentThread + 1) % activeThreads;
		
		inThread = 1;
		
		swapcontext( &mainContext, &threadList[ currentThread].context);

		inThread = 0;

		if( threadList[currentThread].active == 0 ) {
			
			free( threadList[currentThread].stack);

			--activeThreads;

			if( currentThread != activeThreads ) {
				
				threadList[ currentThread] = threadList[ activeThreads];
			}
		
			threadList[ activeThreads].active = 0;
		}
	}
	return;
}


extern void threadJoin(int thread_id, void **result);

//exits the current thread -- closing the main thread, will terminate the program
extern void threadExit(void *result); 

extern void threadLock(int lockNum); 
extern void threadUnlock(int lockNum); 
extern void threadWait(int lockNum, int conditionNum); 
extern void threadSignal(int lockNum, int conditionNum); 

//this 
extern int interruptsAreDisabled;
