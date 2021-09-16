#include <stdio.h>
#include <errno.h>
#include <pthread.h>

#define LAB_SUCCESS ((void*)0)
#define LAB_BAD_PARAM ((void*)1)
#define LAB_FAIL ((void*)2)

#define LAB_THREAD_CREATE_SUCCESS 0

void * run(void * param) {
    if (param == NULL)
        return (LAB_BAD_PARAM);
    
    char *str = (char*)param;
    int i;
    for (i=0; i<10; i++) 
        printf("%d %s\n", i, str);
	return LAB_SUCCESS;
}

int main(int argc, char *argv[]) {
    // since @param attr is null, default attributes for child are set up using pthread_attr_init(3C)
    // https://illumos.org/man/3C/pthread_create usr/src/lib/libc/port/threads/pthread.c
    // https://illumos.org/man/3C/pthread_attr_init usr/src/lib/libc/port/threads/pthr_attr.c
    // https://illumos.org/man/3C/pthread_exit usr/src/lib/libc/port/threads/thr.c

    pthread_t thread;
    pthread_attr_t attr;
    // By calling this we allocate memory for thrattr_t *ap, which is stored in attr->__pthread_attrp,  
    // and has value of *def_thrattr().
    int code = pthread_attr_init(&attr);

    if (code == ENOMEM) {
        fprintf(stderr, "Can't initialise attributes");
        pthread_exit(LAB_FAIL);
    }

    code = pthread_create(&thread, &attr, run, "I was born!");

    if  (code != LAB_THREAD_CREATE_SUCCESS) {
        fprintf(stderr, "Can't create thread");
        pthread_exit(LAB_FAIL);
    }
    run("I gave a birth!");

    // By calling this we free attr->__pthread_attrp and set attr->__pthread_attrp to NULL
    pthread_attr_destroy(&attr);
    /*
    from https://illumos.org/man/3C/pthread_exit: 
    The process exits with an exit status of  0 after the last thread has
    been terminated. The behavior is as if the implementation called exit()
    with a 0 argument at thread termination time.
    */
    pthread_exit(LAB_SUCCESS);  
}
