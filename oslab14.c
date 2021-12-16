#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define LAB_NO_ERROR 0
#define LAB_BAD 1

#define LAB_CANT_CREATE_THREADS 2
#define LAB_THREAD_NOT_STARTED 3
#define LAB_CANT_WAIT_FOR_THREADS 4
#define LAB_CANT_INIT_SEMAPHORE 5
#define LAB_FATAL 6
#define LAB_BAD_ARGS 7

#define LAB_THREADS_NUMBER 2

#define LAB_ITERATION_NUMBER 10
#define LAB_DIFFERENT_STRINGS_NUMBER 16

#define LAB_WAIT 0
#define LAB_POST 1

typedef struct _threadRunParams {
    long i;
    long n;
    long iterations;
    sem_t *sems;
    char *str;
} runParams;
typedef struct _threadLabNode threadLabNode;
struct _threadLabNode {
    runParams params;
    pthread_t thread; 
    int status;
    int section;
};

typedef struct _errorIndexPair errorIndexPair;
struct _errorIndexPair {
    int status;
    long i;
};

void printError(int code, pthread_t thread, char * what) {
    fprintf(stderr, "Error with thr %lu\n%s; %s\n", thread, what, strerror(code));
}

threadLabNode constructNode(runParams p) {
    threadLabNode node;
    node.params = p;
    node.status = LAB_NO_ERROR;
    node.section = LAB_NO_ERROR;
    return node;    
}

int setStatusIfAnyError(int status, int section, threadLabNode* t) {
    if (status != LAB_NO_ERROR) {
        t->status = status;
        t->section = section;
        return 1;
    }
    return 0;
}

void * run(void * param) {
    if (param == NULL) return param;
    threadLabNode *t = (threadLabNode*)param;
    runParams p = t->params;

    sem_t  *sems = p.sems;
    int currentMutex = 1;
    long id = p.i;
    char * str = p.str;

    int status = LAB_NO_ERROR;
    sem_t *semaphoreFirst = &(sems[id]);
    sem_t *semaphoreSecond = &(sems[(id + 1) % 2]);
    for (int i = 0; i < p.iterations; ++i) {
        status = sem_wait(semaphoreSecond);
        if (setStatusIfAnyError(status, LAB_WAIT, t)) return param;
        printf("%d %s\n", i, str);
        status = sem_post(semaphoreFirst);
        if (setStatusIfAnyError(status, LAB_POST, t)) return param;
    }

    return param;
}

threadLabNode* runThreads(threadLabNode *list, long n) {
    for (long i = 0; i < n; ++i) {
        threadLabNode *curr = &(list[i]);
        int code = pthread_create(&(curr->thread), NULL, run, curr);
        curr->status = code;
        if (code != LAB_NO_ERROR)  return curr;
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
        if (status != LAB_NO_ERROR) continue; // wait only for threads who started without problem

        threadLabNode * ret = NULL;
        int code = pthread_join(curr->thread, (void**)(&ret));
        curr->status = code;
        if (code != LAB_NO_ERROR) return curr;
    }

    return NULL;
}

void initThreads(sem_t  *sems, threadLabNode *threads, long n, long iterations) {
    for (long i = 0; i < n; ++i) {
        runParams params = {i, n, iterations, sems, strerror(i % LAB_THREADS_NUMBER)};
        threads[i] = constructNode(params);
    }
}

threadLabNode *checkResults(threadLabNode *threads, long n) {
    for (long i = 0; i < n; ++i) {
        int status = threads[i].status;
        if (status != LAB_NO_ERROR) return &threads[i];
    }

    return NULL;
}

errorIndexPair initSemaphores(sem_t sems[LAB_THREADS_NUMBER]) {
    for (int i = 0; i < LAB_THREADS_NUMBER; ++i) {
        int status = sem_init(&(sems[i]), 0, i);
        if (status != LAB_NO_ERROR) return (errorIndexPair){LAB_CANT_INIT_SEMAPHORE, i};
    }
    return (errorIndexPair){0, 0};
}

int destroySemaphores(sem_t sems[LAB_THREADS_NUMBER]) {
    for (int i = 0; i < LAB_THREADS_NUMBER; ++i) {
        int status = sem_destroy(sems + i);
        if (status != LAB_NO_ERROR) {
            printError(status, pthread_self(), "can't destroy semaphore, fatal error");
            return LAB_FATAL;
        }
    }

    return LAB_NO_ERROR;
}

void describeSectionAndError(int section, int status, pthread_t thread) {
    switch (section)
    {
    case LAB_WAIT:
        printError(status, thread, "problem in waiting");
        break;
    case LAB_POST:
        printError(status, thread, "problem in posting");
        break;
    default:
        printf("shouldn't reach there\n");
        exit(LAB_FATAL);
    }
}

void runChildrenThreads(long iterations) {
    sem_t sems[LAB_THREADS_NUMBER];
    threadLabNode threads[LAB_THREADS_NUMBER];
    
    errorIndexPair result = initSemaphores(sems);
    if (result.status != LAB_NO_ERROR) {
        printError(result.status, pthread_self(), result.i == 0 ? "can't init semaphore attributes" :  "can't init semaphore"); 
        if (destroySemaphores(sems) != LAB_NO_ERROR) exit(LAB_FATAL);
        exit(LAB_CANT_INIT_SEMAPHORE);
    }

    initThreads(sems, threads, LAB_THREADS_NUMBER, iterations);
    threadLabNode * problem = runThreads(threads, LAB_THREADS_NUMBER);
    if (problem != NULL) {
        if (destroySemaphores(sems) != LAB_NO_ERROR) exit(LAB_FATAL);
        printError(problem->status, problem->thread, "thread creation problem, calling exit");
        exit(LAB_CANT_CREATE_THREADS);
    } 

    problem = waitUntilAllThreadsFinish(threads, LAB_THREADS_NUMBER);
    if (problem != NULL) {
        printError(problem->status, problem->thread, "couldn't wait for this thread due to some error");
        if (destroySemaphores(sems) != LAB_NO_ERROR) exit(LAB_FATAL);
        exit(LAB_CANT_WAIT_FOR_THREADS);
    }

    problem = checkResults(threads, LAB_THREADS_NUMBER);
    if (problem != NULL) {
        describeSectionAndError(problem->section, problem->status, problem->thread);
        if (destroySemaphores(sems) != LAB_NO_ERROR) exit(LAB_FATAL);
        exit(LAB_BAD);
    }

    if (destroySemaphores(sems) != LAB_NO_ERROR) exit(LAB_FATAL); 
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
        fprintf(stderr, "bad input: too long or start with 0\n");
        exit(LAB_BAD_ARGS);
    } else if (iterations <= 0) {
        fprintf(stderr,"iterationsNumber must be positive\n");
        exit(LAB_BAD_ARGS);
    } else if (errno) {
        printError(errno, pthread_self(), "can't read number of iterations");
        exit(LAB_BAD_ARGS);
    }

    return iterations;
}

int main(int argc, char *argv[]) {
    long iterations = getIterationsNumber(argc, argv);
    runChildrenThreads(iterations);
    exit(LAB_NO_ERROR);
}
