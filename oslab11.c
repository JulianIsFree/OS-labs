#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define LAB_NO_ERROR 0
#define LAB_SOME_ERROR 1

#define LAB_CANT_CREATE_THREADS 2
#define LAB_THREAD_NOT_STARTED 3
#define LAB_CANT_WAIT_FOR_THREADS 4
#define LAB_BAD_ARGS 5

#define LAB_THREADS_NUMBER 2
#define LAB_MUTEX_NUMBER (LAB_THREADS_NUMBER + 1)

#define LAB_ITERATION_NUMBER 10
#define LAB_DIFFERENT_STRINGS_NUMBER 16

#define LAB_SLEEP 500000
#define LAB_NO_SYNC 0
#define LAB_SYNC 1

#define LAB_STATE_READY 0
#define LAB_STATE_IMMEDIATE 1
#define LAB_STATE_PRINT 2

// #define LAB_DEBUG

// typedef unsigned int pthread_t;
// int pthread_create(pthread_t *thr, void * p,  void *(*start_routine)(void*), void * arg);\
// int pthread_join(pthread_t thread, void **status);
// void pthread_exit(void *value_ptr);

typedef struct _threadRunParams {
    long i;
    long n;
    long iterations;
    pthread_mutex_t *mutexes;
} runParams;
typedef struct _threadLabNode threadLabNode;
struct _threadLabNode {
    runParams params;
    pthread_t thread; 
    int status;
};

void printError(int code, pthread_t thread, char * what) {
    fprintf(stderr, "Error with thr %lu\n%s; %s\n", thread, what, strerror(code));
}

threadLabNode constructNode(runParams p) {
    threadLabNode node;
    node.params = p;
    node.status = LAB_NO_ERROR;
    return node;    
}

int isSync = LAB_NO_SYNC;


void lockOrDie(pthread_mutex_t *mutex) {
    int status = pthread_mutex_lock(mutex) ;
    switch (status)
    {
    case EPERM:
    case EINVAL:
    case EAGAIN:
    case ENOMEM:
        printError(status, pthread_self(), "can't lock");
        exit(1);
        break;
    }
}

void unlockOrDie(pthread_mutex_t * mutex) {
    int status = pthread_mutex_unlock(mutex) ;
    switch (status)
    {
    case EINVAL:
    case EPERM:
    case EOWNERDEAD:
    case ENOTRECOVERABLE:
        printError(status, pthread_self(), "can't lock");
        exit(1);
        break;
    }

}

void yieldOrDie() {
    int status = sched_yield();
    if (status != LAB_NO_ERROR) {
        printError(status, pthread_self(), "can't yield");
        exit(1);
    }
}

void lockAndYieldOrDie(pthread_mutex_t *mutex) {
    lockOrDie(mutex);
    yieldOrDie();
}

void * run(void * param) {
    if (param == NULL) return param;
    threadLabNode *t = (threadLabNode*)param;
    runParams p = t->params;

    pthread_mutex_t *mutexes = p.mutexes;
    int currentMutex = 1;
    long id = p.i;

    if(id != 0) while (!isSync) lockAndYieldOrDie(&mutexes[LAB_STATE_READY]);
    else usleep(LAB_SLEEP);

    lockOrDie(&mutexes[LAB_STATE_PRINT]);
    if (isSync)
        unlockOrDie(&mutexes[LAB_STATE_READY]);

    int index = 0;
    for (int i = 0; i < p.iterations * LAB_MUTEX_NUMBER; i++) {
        lockOrDie(&mutexes[currentMutex]);
        currentMutex = (currentMutex + 1) % LAB_MUTEX_NUMBER;
        unlockOrDie(&mutexes[currentMutex]);
        if (currentMutex == LAB_STATE_PRINT) {
            char * str = strerror(index % LAB_DIFFERENT_STRINGS_NUMBER);
            printf("%ld %ld %s\n", id, index, str);
            isSync = LAB_SYNC;
            index++;
        }
        currentMutex = (currentMutex + 1) % LAB_MUTEX_NUMBER;
    }

    unlockOrDie(&mutexes[LAB_STATE_PRINT]);

    return NULL;
}

threadLabNode* runThreads(threadLabNode *list, long n) {
    for (long i = 0; i < n; ++i) {
        threadLabNode *curr = &(list[i]);
        int code = pthread_create(&(curr->thread), NULL, run, curr);
        curr->status = code;
        
        if (code != LAB_NO_ERROR)  
            return curr;
    }

    return NULL;
}

/**
 * Assumes that threads are joinable and are running or already finished execution
 */
threadLabNode* waitUntilAllThreadsFinish(threadLabNode *runningJoinableThreads, long n) {
    for (long i = 0; i < n; ++i) {
        threadLabNode *curr = &(runningJoinableThreads[i]);
        int status = curr->status;
        
        if (status != LAB_NO_ERROR)
            continue;

        threadLabNode * ret = NULL;
        int code = pthread_join(curr->thread, (void**)(&ret));
        
        curr->status = code;
        if (code != LAB_NO_ERROR)
            return curr;
    }

    return NULL;
}

void initThreads(pthread_mutex_t *mutexes, threadLabNode *threads, long n, long iterations) {
    for (long i = 0; i < n; ++i) {
        runParams params = {i, n, iterations, mutexes};
        threads[i] = constructNode(params);
    }
}

void initMutexes(pthread_mutex_t *mutexes, long n) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    
    int status = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (status != LAB_NO_ERROR) {
        printError(status, pthread_self(), "can't set type for mutex attr");
        exit(1);
    }

    for (int i = 0; i < LAB_MUTEX_NUMBER; ++i) {
        status = pthread_mutex_init(&mutexes[i], &attr);
        if (status != LAB_NO_ERROR) {
            printError(status, pthread_self(), "can't init mutex");
            exit(1);
        }
    }
}

void runMultiThreadCalculations(long n, long iterations) {
    pthread_mutex_t mutexes[n + 1];
    initMutexes(mutexes, n);

    threadLabNode threads[n];
    initThreads(mutexes, threads, n, iterations);
    
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
}

int isCorrect(long v, char * rep) {
    char ns[64];
    sprintf(ns, "%ld", v);
    return strcmp(ns, rep) == 0;
}

long getIterationsNumber(int argc, char **argv) {
    if (argc < 2) return LAB_ITERATION_NUMBER;

    long iterations = strtol(argv[1], (char**)NULL, 10);
    if (isCorrect(iterations, argv[1]) != 1) {
        printf("iterationsNumber must contain only digits and mustn't start with 0\n");
        exit(LAB_BAD_ARGS);
    } else if (iterations <= 0) {
        printf("iterationsNumber must be positive\n");
        exit(LAB_BAD_ARGS);
    } else if (errno) {
        printError(errno, pthread_self(), "can't read number of iterations");
        exit(LAB_BAD_ARGS);
    }

    return iterations;
}

int main(int argc, char *argv[]) {
    long iterations = getIterationsNumber(argc, argv);
    runMultiThreadCalculations(LAB_THREADS_NUMBER, iterations);

    exit(LAB_NO_ERROR);
}
