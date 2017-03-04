
#include <ucontext.h>
#include <stdio.h>
#include <stdlib.h>

ucontext_t main_context, thread_context;

int x = 0;




#define STACK_SIZE 32000

int main (int argc, char **argv)
{
	char *a_new_stack = malloc(STACK_SIZE);

	//get the current context as a starting point
	//for making the new thread context.
	if (getcontext(&thread_context) == -1)
		{ perror("getcontext!"); }

	//use a different stack
	thread_context.uc_stack.ss_sp = a_new_stack;
	thread_context.uc_stack.ss_size = STACK_SIZE;
	//where to go if thread_func ever returns.
	//Don't use uc_link in project 2
	//thread_context.uc_link = &main_context;

	makecontext(&thread_context,
		(void (*) (void))foo, 0, 0);

	swapcontext(&old, &thread_context);

	printf("does this ")
}

void foo(int value)
{
	printf("FOO!\n");
}
