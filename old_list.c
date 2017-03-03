/* queue manipulation functions */    
    
static void put( T thread, T* queue) {    
    assert(t);    
    assert(t->inqueue == NULL && thread->link == NULL);    
    if(*queue) {    
        thread->link = (*queue)->link;    
        (*queue)->link = thread;    
    }    
    else {    
        thread->link = thread;    
    }    
    
    *queue = thread;    
    thread->inqueue = queue;    
}    

static T get(T *queue) {
    T thread;
    assert( !isempty( *queue));
    thread = ( *queue)->link;
    if( thread == *queue) {
        *queue = NULL;
    }
    else {
        (*queue)->link = thread->link;
    }
    assert( thread->inqueue == queue);
    thread->link = NULL;
    thread->inqueue = NULL;
    return thread;
}

static void delete( T thread, T *queue) {
    T temp;
    assert( thread->link && thread->inqueue == q);
    assert( !isempty(*queue));
    for( temp = *queue; temp->link != thread; temp = temp->link)
        ;
    if( temp == thread) {
        *queue = NULL;
    }
    else {
        temp->link = thread->link;
        if( *queue == thread) {
            *queue = temp;
        }
    }

    thread->link = NULL;
    thread->inqueue = NULL;
}


