#pragma once
#include "list.h"
#include "mythreads.h"
#include <unistd.h>                                                                                              
#include <sys/types.h>                                                                                           
#include <sys/syscall.h>                                                                                         
#include <ucontext.h>                                                                                            
#include <stdlib.h>                                                                                              
#include <stdio.h>                                                                                               
#include <string.h>                                                                                              
#include <signal.h>                                                                                              
#include <assert.h>                                                                                              
#include <sys/time.h>      

typedef enum state { running = 0, blocked = 1, dead = 2} State_t;

typedef struct thread_t {                                                                                            
    ucontext_t context; /* tcb vars */                                                                        
    thFuncPtr function;                                                                                       
    void* args;                                                                                               
    void* results;                                                                                            
    size_t id;                                                                                                
	State_t state;
	int waiting_on;
}thread_t;          

