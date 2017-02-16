#include "mythreads.h"
#include "list.h"

//holds each thread in a queue
static node_t* threadList;

//number of active threads
static size_t activeThreads;

//index of current thread
static size_t currentThread;

//boolean if in main(1) or in thread(0)
static uint8_t inThread;

static ucontext_t mainContext;

//holds each thread in queue
extern void threadInit() {

	activeThreads = 0;
	currentThread = 0;
	threadList = NULL;
	
}

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
