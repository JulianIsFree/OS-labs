#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#define LAB_NO_ERROR 0
#define LAB_SOME_ERROR 1
#define LAB_CANT_CREATE_THREADS 2
#define LAB_THREAD_NOT_STARTED 3
#define LAB_CANT_WAIT_FOR_THREADS 4

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

void printError(int code, pthread_t thread, char * what) {
    fprintf(stderr, "Error with thr %lu\n %s; %s\n", thread, what, strerror(code));
}

runParams makeStringArrayOfNumbers(char startchar, int n) {
    char ** arr = malloc(sizeof(char*) * n);

    for (int i = 0; i < n; ++i) {
        arr[i] = malloc(sizeof(char) * i + 1);
        
        for (int j = 0; j < i; ++j) {
            arr[i][j] = startchar + (j % 256);
        }
        arr[i][i] = '\0';
    }

    runParams params;
    params.count = n;
    params.strings = arr;

    return params;
}

threadLabNode * createThreadsByNumber(int n, int *arr) {
    threadLabNode * head = NULL;

    for (int i = 0; i < n; ++i) {
        runParams params = makeStringArrayOfNumbers('a' + (char)i, arr[i]);
        threadLabNode * node = createNode(params);
        addFirst(&head, node);
    }

    return head;
}

threadLabNode * runThreads(threadLabNode *list) {
    threadLabNode *curr = list; 

    while (curr != NULL) {
        int code = pthread_create(&(curr->thread), NULL, run, curr);
        curr->status = code;
        if (code != LAB_NO_ERROR) {
            return curr;
        }
        curr = curr->next;
    }

    return NULL;
}

void freeLabNode(threadLabNode* node) {
    freeArr(node->params.strings, node->params.count);
    free(node);
}

/**
 * Assumes that threads are joinable and are running or already are finished execution
 */
threadLabNode* waitUntilAllThreadsFinish(threadLabNode *runningJoinableThreads) {
    threadLabNode *curr = runningJoinableThreads;

    while (curr != NULL) {
        int status = curr->status;
        if (status != LAB_NO_ERROR) {
            threadLabNode * ret = NULL;
            int code = pthread_join(curr->thread, (void**)(&ret));
            curr->status = code;

            if (code == LAB_NO_ERROR) {
                /*No errors, it's just fine as ESRCH*/
            } else if (code == ESRCH) {
                /*It's fine if thread is already finished or doesn't exist at all*/
            } else if (code == EINVAL || code == EDEADLK) {
                return curr;
            }
        }
        curr = curr->next;
    }

    return NULL;
}

int main(int argc, char *argv[]) {
    // no static
    // make code scalable
    // run_child_thread must return code errors, with semantic and handling
    
    // + auto thread creation

    threadLabNode * head = NULL;
    if (argc == 1) {
        addFirst(&head, createNode(makeStringArray(3, "Boys", "are", "stronger")));
        addFirst(&head, createNode(makeStringArray(4, "Girls", "are", "more", "beautiful")));
        addFirst(&head, createNode(makeStringArray(2, "I'm", "young")));
        addFirst(&head, createNode(makeStringArray(3, "Olds", "are", "older")));
    } else {
        int n = atoi(argv[1]); 
        int * arr = malloc(sizeof(int) * n);
        for (int i = 0; i < n; ++i) 
            arr[i] = atoi(argv[2 + i]);
        head = createThreadsByNumber(n, arr);
        free(arr);
    }
    
    threadLabNode * problem = runThreads(head);
    if (problem != NULL) {
        printError(problem->status, problem->thread, "thread creation problem, calling exit");
        freeList(head);
        exit(LAB_CANT_CREATE_THREADS);
    } 

    problem = waitUntilAllThreadsFinish(head);
    if (problem != NULL) {
        printError(problem->status, problem->thread, "couldn't wait for this thread due to some error");    
        freeList(head);
        exit(LAB_CANT_WAIT_FOR_THREADS);
    }

    freeList(head);
    pthread_exit(LAB_NO_ERROR);  
}
