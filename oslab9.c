#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define LAB_NO_ERROR 0
#define LAB_SOME_ERROR 1

// #define LAB_DEBUG

#define LAB_ITERATION_NUMBER 100000000

// typedef unsigned int pthread_t;
// int pthread_create(pthread_t *thr, void * p,  void *(*start_routine)(void*), void * arg);\
// int pthread_join(pthread_t thread, void **status);
// void pthread_exit(void *value_ptr);

typedef struct _threadRunParams {
    int startIndex;
    int count;
    double result;
} runParams;

typedef struct _threadLabNode threadLabNode;
struct _threadLabNode {
    runParams params;
    pthread_t thread; 
    int status;
    threadLabNode *next;
};

threadLabNode * createNode(runParams p) {
    threadLabNode * node = malloc(sizeof(threadLabNode));
    node->params = p;
    node->next = NULL;
    node->status = LAB_NO_ERROR;
    return node;
}

void addFirst(threadLabNode ** head, threadLabNode * node) {
    node->next = *head;
    *head = node;
}

void freeList(threadLabNode *node) {
    while (node != NULL) {
        threadLabNode * curr = node;
        node = node->next;
        free(curr);
    }
}

runParams buildParams(int startIndex, int count) {
    return (runParams){startIndex, count, 0.0};
}

double f(int i) {
    return ((i % 2 == 0) ? 1.0 : -1.0) / (2*i + 1);
}

void * run(void * param) {
    if (param == NULL)
        return param;
    
    threadLabNode * tn = (threadLabNode*)param;
    runParams p = tn->params;
    
    double res = 0;
    int x = p.startIndex;
    
    while (true) {
        tn->params.result += f(x);
        x += p.count;
        pthread_testcancel();
    }

    return param;
}

void printError(int code, pthread_t thread, int num, char * what) {
    fprintf(stderr, "Error with thr %lu, num in list %d\n %s; %s\n", thread, num, what, strerror(code));
}

int runThreads(threadLabNode *list) {
    threadLabNode *curr = list;
    while (curr != NULL) {
        int code = pthread_create(&(curr->thread), NULL, run, curr);
        curr->status = code;
        curr = curr->next;
    }
}

void freeLabNode(threadLabNode* node) {
    free(node);
}

threadLabNode *list = NULL;
void catchReturnAndPrintErrorsAndFreeList(threadLabNode *list) {
    threadLabNode *curr = list;
    int count = 0;
    double res = 0;

    while (curr != NULL) {
        int code = pthread_cancel(curr->thread);
        if (code == ESRCH) {
            printError(code, curr->thread, count, "can't cancel thread");
        }

        count++;
        curr = curr->next;
    }

    count = 0;
    curr = list;
    while (curr != NULL) {
        int status = curr->status;

        if (status != LAB_NO_ERROR) {
            printError(status, curr->thread, count, "error on creating thread, no handle");
        } else {
            threadLabNode * ret = NULL;
            int code = pthread_join(curr->thread, (void**)(&ret)); 
            if (code != LAB_NO_ERROR) {
                printError(code, curr->thread, count, "");
            } else {
                res += curr->params.result;
            }
        }
        
        curr = curr->next;
        count ++;
    }

    freeList(head);
    printf("pi = %lf\n", res);
}

threadLabNode *initNodes(int threadsNumber) {
    threadLabNode * head = NULL;

    for (int i = 0; i < threadsNumber; ++i) {
        addFirst(&head, createNode(buildParams(i, threadsNumber)));
    }

    return head;
}

int main(int argc, char *argv[]) {
    // no static
    // make code scalable
    // run_child_thread must return code errors, with semantic and handling
    int n = 4;
    int iterations = LAB_ITERATION_NUMBER;
    if (argc >= 2) {
        n = atoi(argv[1]);
    } 

    if (argc >= 3) {
        iterations = atoi(argv[2]);
    }

    printf("threads = %d\n", n);
    threadLabNode * head = initNodes(n);
    runThreads(head);
    double res = 4 * catchReturnAndPrintErrors(head);

    pthread_exit(LAB_NO_ERROR);  
}
