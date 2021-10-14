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
} runParam;

void * run(void * param) {
    if (param == NULL)
        return param;
    runParam *p = (runParam*)param;
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
    int n1 = 10;
    int n2 = 10; // number of repeations for parent and child
    
    if (argc >= 2) {
        n1 = atoi(argv[1]);
    }
    if (argc >= 3) {
        n2 = atoi(argv[2]);
    }

    runParam child = {"I was  born!", n1};
    runParam parent = {"I gave a birth!", n2};

    pthread_t childThreadId;
    pthread_attr_t attr;
    int code = pthread_attr_init(&attr);
    if (code == ENOMEM) {
        print_error(code, childThreadId, "can't init pthread_attr for child thread");
        exit(LAB_FAIL);
    }

    code = pthread_create(&childThreadId, &attr, run, &child);

    if  (code != LAB_THREAD_CREATE_SUCCESS) {
        print_error(code, childThreadId, "");
        exit(LAB_FAIL);
    }

    run(&parent);

    code = pthread_join(childThreadId, NULL);
    if (code == EINVAL || code = EDEADLK) {
        print_error(code, childThreadId, "can't join to child thread, something unusual happened");
        exit(LAB_FAIL);
    }
    
    pthread_attr_destroy(&attr);
    pthread_exit(LAB_SUCCESS);  
}
