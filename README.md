#User Mode Thread Library

##Thread Management

###	threadinit

initialize library

###threadCreate

create a new thread, will have a stack size of STACK_SIZE

executes the specified function with the arguement ( void * )

###threadYield

causes the current running process to "yield" the processor to the next runnable thread

saves current thread context and select the next thread for execution

###threadJoin

waits until the thread corresponding to id exits

if results aren't NULL then the thread function's return value is stored as the 
	address pointed to by result

if the thread specified by id has already exited or does not exist then
	the call should return immediately

store the result of all exited threads so you can retreive their results

you do not need to ever free the results ( for this assignment only!)

###threadExit

causes the current thread to exit

arguement passed to threadExit is the thread's return value which should be passed to
	any calls to to threadJoin by other threads waiting on this thread


##Sychronization

###threadLock
	
This function blocks (waits) until it is able to acquire the specified lock.

Your library should allow other threads to continue to execute concurrently
	while one or more threads are waiting for locks
	
The parameter passed to the function indicates which lock is to be locked 
	This parameter is not the lock itself

###threadUnlock

