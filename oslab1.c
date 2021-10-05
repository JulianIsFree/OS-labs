#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define LAB_THREAD_CREATE_SUCCESS 0

#define LAB_SUCCESS 0
#define LAB_FAIL 1

typedef struct _lparam {
    char *str;
    int count;
} lparam;

void * run(void * param) {
    if (param == NULL)
        return param;
    lparam *p = (lparam*)param;
    char *str = p->str;
    int i;
    for (i=0; i<p->count; i++) 
        printf("%d %s\n", i, str);
	return param;
}

void print_error(int code, pthread_t thread, char * what) {
    fprintf(stderr, "Error: %lu %s: %s\n", thread, what, strerror(code));
}

int main(int argc, char *argv[]) {
    // since @param attr is null, default attributes for child are set up using pthread_attr_init(3C)
    // https://illumos.org/man/3C/pthread_create usr/src/lib/libc/port/threads/pthread.c
    // https://illumos.org/man/3C/pthread_attr_init usr/src/lib/libc/port/threads/pthr_attr.c
    // https://illumos.org/man/3C/pthread_exit usr/src/lib/libc/port/threads/thr.c

    pthread_t thread;
    pthread_attr_t attr;
    static lparam child = {"I was  born!", 10};
    static lparam parent = {"I gave a birth!", 10};

    // By calling this we allocate memory for thrattr_t *ap, which is stored in attr->__pthread_attrp,  
    // and has value of *def_thrattr().
    int code = pthread_attr_init(&attr);
    if (code == ENOMEM) {
        print_error(code, thread, "can't init attr");
        exit(LAB_FAIL);
    }

    code = pthread_create(&thread, &attr, run, &child);

    if  (code != LAB_THREAD_CREATE_SUCCESS) {
        print_error(code, thread, "can't create thread");
        exit(LAB_FAIL);
    }
    run(&parent);

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
