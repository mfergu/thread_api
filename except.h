/* The Except interface wraps the setjmp/longjmp facility in a set of macros and functions
 * that provides an exception capability structure*/
#ifndef EXCEPT_INCLUDED
#define EXCEPT_INCLUDED

#include <setjmp.h>

#define T Except_T
/* The macros and functions in the Except interface collaborate to maintain a stack
 * of structures that record the exception state and the instantiated handlers. */

// struct that holds reason of exception
typedef struct T {
	const char *reason;
} T;

typedef struct Except_Frame Except_Frame;


struct Except_Frame {
	Except_Frame *prev; //points to the previous stack frame
	jmp_buf env; //env field of this structure is a jmp_buf, which is used by setjmp and longjmp
	const char *file; //file name of exception
	int line; //line number in file of exception
	const T *exception; //except handler tests this field to determine which handler applies
};

enum { Except_entered=0, Except_raised, Except_handled, Except_finalized};

extern Except_Frame *Except_stack;

extern const Except_T Assert_Failed;

void Except_raise(const T *e, const char *file,int line);


#define RAISE(e) Except_raise(&(e), __FILE__, __LINE__)
#define RERAISE Except_raise(Except_frame.exception, \
	Except_frame.file, Except_frame.line)
#define RETURN switch (Except_stack = Except_stack->prev,0) default: return

/* The macros TRY, EXCEPT, ELSE, FINALLY, and END_TRY collaborate to
 * translate a TRY-EXCEPT statement into a do-while statement */

	/*The first return from setjmp sets Except_flag to Except_entered,
	 * which indicates that a TRY statement has been entered and an exception
	 * frame has been pushed onto the exception stack. */
#define TRY do { \
	volatile int Except_flag; \
	Except_Frame Except_frame; \
	Except_frame.prev = Except_stack; \
	Except_stack = &Except_frame;  \
	Except_flag = setjmp(Except_frame.env); \
	if (Except_flag == Except_entered) {
#define EXCEPT(e) \
		if (Except_flag == Except_entered) Except_stack = Except_stack->prev; \
	} else if (Except_frame.exception == &(e)) { \
		Except_flag = Except_handled;
#define ELSE \
		if (Except_flag == Except_entered) Except_stack = Except_stack->prev; \
	} else { \
		Except_flag = Except_handled;
#define FINALLY \
		if (Except_flag == Except_entered) Except_stack = Except_stack->prev; \
	} { \
		if (Except_flag == Except_entered) \
			Except_flag = Except_finalized;
#define END_TRY \
		if (Except_flag == Except_entered) Except_stack = Except_stack->prev; \
		} if (Except_flag == Except_raised) RERAISE; \
} while (0)
#undef T
#endif
