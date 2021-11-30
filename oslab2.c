#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define LAB_SUCCESS ((void*)0)
#define LAB_BAD_PARAM ((void*)1)
#define LAB_FAIL ((void*)2)

#define LAB_THREAD_CREATE_SUCCESS 0
#define LAB_THREAD_JOIN_SUCCESS 0

void * run(void * param) {
    if (param == NULL)
        return (LAB_BAD_PARAM);
    
    char *str = (char*)param;
    int i;
    for (i=0; i<10; i++) 
        printf("%d %s\n", i, str);
	return LAB_SUCCESS;
}

void print_error(int code, pthread_t thread, char * what) {
    fprintf(stderr, "Error: %lu %s: %s\n", thread, what, strerror(code));
}

int main(int argc, char *argv[]) {
    pthread_t thread;
    pthread_attr_t attr;

    int code = pthread_attr_init(&attr);
    if (code == ENOMEM) {
        fprintf(stderr, "Can't initialise attributes");
        pthread_exit(LAB_FAIL);
    }

    code = pthread_create(&thread, &attr, run, "I was born!");
    
    if  (code != LAB_THREAD_CREATE_SUCCESS) {
        print_error(code, thread, "can't create thread");
        pthread_exit(LAB_FAIL);
    }

    int *status;
    code = pthread_join(thread, (void**)(&status));
    if (code == EINVAL) {
        print_error(code, thread, "target thread is detached");
        pthread_exit(LAB_FAIL);
    }
    if (code == ESRCH) {
        print_error(code, thread, "someone stole your sweet role");
        pthread_exit(LAB_FAIL);
    }
    if (status == LAB_BAD_PARAM) {
        print_error(code, thread, "bad params for joined thread");
        pthread_exit(LAB_FAIL);
    }
    if (status != LAB_SUCCESS) {
        fprintf(stderr, "Unknown status: %d", status);
    }

    run("I gave a birth!");

    // read oslab1.c for comments
    pthread_attr_destroy(&attr);
    pthread_exit(LAB_SUCCESS);  
}
