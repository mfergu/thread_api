#undef assert
#ifdef NDEBUG
#define assert(e) ((void)0)
#else
#include "except.h"
/* assert(e) evaluates e and if e is zero, writes diagnostic information
 * on the standard error and aborts execution by calling the standard
 * library function abort */
extern void assert(int e);
#define assert(e) ((void)((e)||(RAISE(Assert_Failed),0)))
#endif
