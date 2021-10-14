#include <stdio.h>

#include <signal.h>
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

#define LAB_ITERATION_NUMBER 100000000

// #define LAB_DEBUG

// typedef unsigned int pthread_t;
// int pthread_create(pthread_t *thr, void * p,  void *(*start_routine)(void*), void * arg);\
// int pthread_join(pthread_t thread, void **status);
// void pthread_exit(void *value_ptr);

typedef struct _threadRunParams {
    unsigned int startIndex;
    unsigned int count;
    int iterationsNumber;
    double result;
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

double f(unsigned int  i) {
    return ((i % 2 == 0) ? 1.0 : -1.0) / (2.0*(double)i + 1.0);
}

static int doRun = 1;

/**
 * this function should not be called from anywhere except signal handler
 * since it affects work of different threads
 */
void sigcatch(int sig) {
    doRun = 0;
}

void * run(void * param) {
    if (param == NULL)
        return param;
    
    threadLabNode * tn = (threadLabNode*)param;
    runParams p = tn->params;
    
    unsigned int x = p.startIndex;
    while(doRun) {
        for (int i = 0; i < p.iterationsNumber; ++i) {
            tn->params.result += f(x);
            x += p.count;
        }
    }
#ifdef LAB_DEBUG
    printf("%d %d %.15g\n", p.startIndex, x / p.count,4 * tn->params.result);
#endif
    return param;
}

void printError(int code, pthread_t thread, char * what) {
    fprintf(stderr, "Error with thr %lu\n %s; %s\n", thread, what, strerror(code));
}

threadLabNode* runThreads(threadLabNode *list, int n) {
    for (int i = 0; i < n; ++i) {
        threadLabNode *curr = &(list[i]);
        int code = pthread_create(&(curr->thread), NULL, run, curr);
        curr->status = code;
        if (code != LAB_NO_ERROR) {
            return curr;
        }
    }

    return NULL;
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
            } else if (code == ESRCH) {
                /*It's fine if thread is already finished or doesn't exist at all*/
            } else if (code == EINVAL || code == EDEADLK) {
                return curr;
            }
        } else {
            /*Means thread wasn't started or already was in this *good* branch*/
        }
    }

    return NULL;
}

double collectResults(threadLabNode *finishedThreads, int n) {
    double res = 0;

    for (int i = 0; i < n; ++i) {
        res += finishedThreads[i].params.result;
    }

    return res;
}

void initThreads(threadLabNode *threads, int n, int iterations) {
    for (int i = 0; i < n; ++i) {
        runParams params = {(unsigned int)i, (unsigned int)n, iterations, 0.0};
        threads[i] = constructNode(params);
    }
}

int main(int argc, char *argv[]) {
    // no static
    // make code scalable
    // run_child_thread must return code errors, with semantic and handling
    int n;
    int iterations;
    
    if (argc >= 2) {
        n = atoi(argv[1]);
        if (argc >= 3)
            iterations = atoi(argv[2]);
        else 
            iterations = LAB_ITERATION_NUMBER;
    } else {
        printf("args: threadsNumber [ iterationsNumber ]\n");
        exit(LAB_BAD_ARGS);
    }

    struct sigaction act;
    act.sa_handler = sigcatch;    
    //https://illumos.org/man/2/sigaction
    if (sigaction(SIGINT, &act, NULL) == EINVAL) {
        printError(EINVAL, pthread_self(), "can't set signal SIGINT for main thread");
        exit(LAB_SOME_ERROR);
    }
    
    threadLabNode threads[n];
    initThreads(threads, n, iterations);
    
    threadLabNode * problem = runThreads(threads, n);
    if (problem != NULL) {
        printError(problem->status, problem->thread, "thread creation problem, calling exit");
        exit(LAB_CANT_CREATE_THREADS);
    } 

    problem = waitUntilAllThreadsFinish(threads, n);
    if (problem != NULL) {
        printError(problem->status, problem->thread, "couldn't wait for this thread due to some error");
        exit(LAB_CANT_WAIT_FOR_THREADS);
    }

    double pi = 4 * collectResults(threads, n);
    printf("pi=%.15g\n", pi);

    pthread_exit(LAB_NO_ERROR);  
}
