#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define LAB_THREAD_CREATE_SUCCESS 0

#define LAB_SUCCESS 0
#define LAB_FAIL 1

typedef struct _lparam {
    char **str;
    int count;
} lparam;

void * run(void * param) {
    if (param == NULL)
        return param;
    lparam *p = (lparam*)param;
    char **str = p->str;
    int i;
    for (i=0; i<p->count; i++) 
        printf("%d %s\n", i + 1, str[i]);
	return param;
}

void print_error(int code, pthread_t thread, char * what) {
    fprintf(stderr, "Error: %lu %s: %s\n", thread, what, strerror(code));
}

void run_child_thread(lparam *child) {
    // all links in oslab1.c
    pthread_t thread;
    pthread_attr_t attr;

    int code = pthread_attr_init(&attr);
    if (code == ENOMEM) {
        print_error(code, thread, child->str);
        exit(LAB_FAIL);
    }

    code = pthread_create(&thread, &attr, run, child);
    if  (code != LAB_THREAD_CREATE_SUCCESS) {
        print_error(code, thread, child->str);
        exit(LAB_FAIL);
    }

    pthread_attr_destroy(&attr);
}

int main(int argc, char *argv[]) {
    static lparam parent = {{"I gave a birth!", "3 children"}, 2};
    static lparam child1 = {{"I'm eldest!", "Second and Third are younger"}, 2};
    static lparam child2 = {{"I'm middle!", "Third is younger"}, 2};
    static lparam child3 = {{"I'm youngest!"}, 1};
    run_child_thread(&child1);
    run_child_thread(&child2);
    run_child_thread(&child3);
    
    run(&parent);
    
    pthread_exit(LAB_SUCCESS);  
}
