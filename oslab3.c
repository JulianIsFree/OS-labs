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
#define LAB_BAD_ARGS 5
#define LAB_BAD_ALLOC 6

// typedef unsigned int pthread_t;
// int pthread_create(pthread_t *thr, void * p,  void *(*start_routine)(void*), void * arg);\
// int pthread_join(pthread_t thread, void **status);
// void pthread_exit(void *value_ptr);
//  pthread_t pthread_self(void);

typedef struct _threadRunParams {
    char **strings;
    int count;
} runParams;

typedef struct _threadLabNode threadLabNode;
struct _threadLabNode {
    runParams params;
    pthread_t thread; 
    int status;
};

threadLabNode constructNode(runParams p) {
    threadLabNode node;
    node.params = p;
    node.status = LAB_NO_ERROR;
    return node;    
}

void freeParams(runParams param) {
    char ** arr = param.strings;
    if (arr == NULL)
        return;

    int count = param.count;
    for (int i = 0; i < count; ++i) {
        free(arr[i]);
    }

    free(arr);
}

void * run(void * param) {
    if (param == NULL)
        return param;
    
    threadLabNode * tn = (threadLabNode*)param;
    runParams p = tn->params;

    for (int i = 0; i < p.count; ++i) {
        printf("%d %d %s\n",tn->thread, i, p.strings[i]);
        if (errno != LAB_NO_ERROR) {
            printError(errno, pthread_self(), "");
            break;
        }
    } 
        
	return param;
}

void printError(int code, pthread_t thread, char * what) {
    fprintf(stderr, "Error with thr %lu\n %s; %s\n", thread, what, strerror(code));
}

runParams makeStringArrayOfLength(int n) {
    char ** arr = malloc(sizeof(char*) * n);
    if (arr == null)
        return (runParams) {NULL, 0};

    for (int i = 0; i < n; ++i) {
        char * str = strerror((i % 10) + 1);
        int len = strlen(str);
        arr[i] = malloc(sizeof(char) * len + 1);

        if (arr[i] == NULL) 
            return (runParams){NULL, 0};

        memcpy(arr[i], str, sizeof(char) * len);
        arr[i][len] = '\0';
    }

    runParams params;
    params.strings = arr;
    params.count = n;
    return params;
}

int initThreads(threadLabNode *threads, int n, int *arr) {
    for (int i = 0; i < n; ++i) {
        runParams params = makeStringArrayOfLength(arr[i]);
        if (params.strings == NULL && arr[i] != 0) 
            return i + 1;
        
        threads[i] = constructNode(params);
    }

    return LAB_NO_ERROR;
}

threadLabNode* runThreads(threadLabNode *list, int n) {
    for (int i = 0; i < n; ++i) {
        threadLabNode *curr = &(list[i]);
        int code = pthread_create(&(curr->thread), NULL, run, curr);
        curr->status = code;
        if (code != LAB_NO_ERROR)
            return curr;
    }

    return NULL;
}

void freeThreads(threadLabNode *arr, int n) {
    for(int i = 0; i < n; ++i) {
        freeParams(arr[i].params);
    }
}

/**
 * Assumes that threads are joinable and are running or already finished execution
 */
threadLabNode* waitUntilAllThreadsFinish(threadLabNode *runningJoinableThreads, int n) {
    for (int i = 0; i < n; ++i) {
        threadLabNode *curr = &(runningJoinableThreads[i]);
        int status = curr->status;
        if (status == LAB_NO_ERROR) {
            threadLabNode * ret = NULL;
            int code = pthread_join(curr->thread, (void**)(&ret));
            curr->status = code;

            if (code == LAB_NO_ERROR) {
                /*No errors, it's just fine as ESRCH*/
            } 
            #ifdef LAB_ALLOW_MN_JOIN // whatever it's allowed for n threads to wait for same m threads or not
            else if (code == ESRCH) {
                /*It's fine if thread is already finished or doesn't exist at all*/
            } else if (code == EINVAL || code == EDEADLK) {
                return curr;
            }
            #else 
            else {
                return curr;
            }
            #endif
        } else {
            /*Means thread wasn't started or already was in this *good* branch*/
        }
    }

    return NULL;
}

void fillArray(int * arr, int n, char **argv) {
    for (int i = 0; i < n; ++i) {
        arr[i] = atoi(argv[i]);
    }
}

int main(int argc, char *argv[]) {
    // no static
    // make code scalable
    // run_child_thread must return code errors, with semantic and handling
    int n;
    if (argc >= 2) {
        n = atoi(argv[1]);
        if (argc - 2 < n) {
            printf("Not enough arguments: %d required, %d got\n", n, argc - 2);
            exit(LAB_BAD_ARGS);
        }
    } else {
        printf("Expected arguments: n r_1 ... r_n\n");
        exit(LAB_BAD_ARGS);
    }
    
    int arr[n];
    fillArray(arr, n,  argv + 2);

    threadLabNode threads[n];
    int index;
    if ((index = initThreads(threads, n, arr)) != LAB_NO_ERROR) {
        printError(errno, pthread_self(), "can't allocate memory for strings for threads");
        freeThreads(threads, index - 1);
        exit(LAB_BAD_ALLOC);
    }
    
    threadLabNode * problem = runThreads(threads, n);
    if (problem != NULL) {
        printError(problem->status, problem->thread, "thread creation problem, calling exit");
        freeThreads(threads, n);
        exit(LAB_CANT_CREATE_THREADS);
    } 

    problem = waitUntilAllThreadsFinish(threads, n);
    if (problem != NULL) {
        printError(problem->status, problem->thread, "couldn't wait for this thread due to some error");
        freeThreads(threads, n);
        exit(LAB_CANT_WAIT_FOR_THREADS);
    }

    freeThreads(threads, n);
    pthread_exit(LAB_NO_ERROR);  
}
