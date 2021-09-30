#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define LAB_NO_ERROR 0
#define LAB_SOME_ERROR 1

// typedef unsigned int pthread_t;
// int pthread_create(pthread_t *thr, void * p,  void *(*start_routine)(void*), void * arg);\
// int pthread_join(pthread_t thread, void **status);
// void pthread_exit(void *value_ptr);

typedef struct _threadRunParams {
    char **strings;
    int count;
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

void freeArr(char ** arr, int count) {
    if (arr == NULL)
        return;

    for (int i = 0; i < count; ++i) {
        free(arr[i]);
    }

    free(arr);
}

void freeList(threadLabNode *node) {
    while (node != NULL) {
        threadLabNode * curr = node;
        node = node->next;
        freeArr(curr->params.strings, curr->params.count);
        free(curr);
    }
}

runParams makeStringArray(int n, ...) {
    char * p = (char*)&n;
    char ** arr = malloc(sizeof(char*) * n);

    p += sizeof(char*);
    for (int i = 0; i < n; ++i) {
        char * str = *((char**)p);
        arr[i] = strcpy(malloc(strlen(str)), str);
        p += sizeof(char*);
    }

    runParams par = {arr, n};
    return par;
}

void * run(void * param) {
    if (param == NULL)
        return param;
    
    threadLabNode * tn = (threadLabNode*)param;
    runParams p = tn->params;

    for (int i = 0; i < p.count; ++i) 
        printf("%d %s\n", i, p.strings[i]);

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
    freeArr(node->params.strings, node->params.count);
    free(node);
}

void printErrors(threadLabNode *list) {
    threadLabNode *curr = list;
    int count = 0;

    while (curr != NULL) {
        int status = curr->status;

        if (status != LAB_NO_ERROR) {
            printError(status, curr->thread, count, "error on creating thread, no handle");
        } else {
            threadLabNode * ret = NULL;
            int code = pthread_join(curr->thread, (void**)(&ret));    
            if (code != LAB_NO_ERROR) {
                printError(code, curr->thread, count, "");
            }
        }
        
        curr = curr->next;
        count ++;
    }
}

int main(int argc, char *argv[]) {
    // no static
    // make code scalable
    // run_child_thread must return code errors, with semantic and handling

    threadLabNode * head = NULL;
    addFirst(&head, createNode(makeStringArray(3, "Boys", "are", "stronger")));
    addFirst(&head, createNode(makeStringArray(4, "Girls", "are", "more", "beautiful")));
    addFirst(&head, createNode(makeStringArray(2, "I'm", "young")));
    addFirst(&head, createNode(makeStringArray(3, "Olds", "are", "older")));

    runThreads(head);
    printErrors(head);
    freeList(head);

    pthread_exit(LAB_NO_ERROR);  
}
