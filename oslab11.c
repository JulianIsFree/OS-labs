#include <stdio.h>
#include <errno.h>
#include <pthread.h>
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
#define LAB_CANT_INIT_MUTEX 5
#define LAB_FATAL 6
#define LAB_BAD_ARGS 7

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

#define LAB_LOCK_SECTION 0
#define LAB_UNLOCK_SECTION 1
#define LAB_PSEUDOSYNC_SECTION 2
#define LAB_END_SECTION 3
typedef struct _threadRunParams {
    long i;
    long n;
    long iterations;
    pthread_mutex_t *mutexes;
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

int lockAndYield(pthread_mutex_t *mutex) {
    int status = pthread_mutex_lock(mutex);
    if (status != EDEADLK && status != LAB_NO_ERROR) return status; 
    return sched_yield();
}

int setStatusIfAnyError(int status, int section, threadLabNode* t) {
    if (status != LAB_NO_ERROR) {
        t->status = status;
        t->section = section;
        return 1;
    }
    return 0;
}

int isSync = LAB_NO_SYNC; 
void * run(void * param) {
    if (param == NULL) return param;
    threadLabNode *t = (threadLabNode*)param;
    runParams p = t->params;

    pthread_mutex_t *mutexes = p.mutexes;
    int currentMutex = 1;
    long id = p.i;
    char * str = p.str;

    int status = LAB_NO_ERROR;

    if (id == 0) usleep(LAB_SLEEP);
    while (!isSync && id != 0)  {
        status = lockAndYield(&mutexes[LAB_STATE_READY]);
        if (setStatusIfAnyError(status, LAB_PSEUDOSYNC_SECTION, t)) return param;
    }
    
    status = pthread_mutex_lock(&mutexes[LAB_STATE_PRINT]);
    if (setStatusIfAnyError(status, LAB_LOCK_SECTION, t)) return param;

    if (isSync)  {
        status = pthread_mutex_unlock(&mutexes[LAB_STATE_READY]);
        if (setStatusIfAnyError(status, LAB_UNLOCK_SECTION, t)) return param;
    }

    for (long i = 0; i < p.iterations * LAB_MUTEX_NUMBER; i++) {
        status = pthread_mutex_lock(&mutexes[currentMutex]); 
        if (setStatusIfAnyError(status, LAB_LOCK_SECTION, t)) return param;

        currentMutex = (currentMutex + 1) % LAB_MUTEX_NUMBER;
        
        status = pthread_mutex_unlock(&mutexes[currentMutex]);  
        if (setStatusIfAnyError(status, LAB_UNLOCK_SECTION, t)) return param;
        
        if (currentMutex == LAB_STATE_PRINT) {
            printf("%ld %ld %s\n", id, i / LAB_MUTEX_NUMBER, str);
            if (!isSync) isSync = LAB_SYNC;
        }
        currentMutex = (currentMutex + 1) % LAB_MUTEX_NUMBER;
    }

    status = pthread_mutex_unlock(&mutexes[LAB_STATE_PRINT]);
    (void) setStatusIfAnyError(status, LAB_END_SECTION, t);
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

void initThreads(pthread_mutex_t *mutexes, threadLabNode *threads, long n, long iterations) {
    for (long i = 0; i < n; ++i) {
        runParams params = {i, n, iterations, mutexes, strerror(i % LAB_THREADS_NUMBER)};
        threads[i] = constructNode(params);
    }
}

// Must be called only for inited mutexes
int deinitMutexes(pthread_mutex_t *mutexes, long n) {
    int fatal = 0;
    for (long i = 0; i < n; ++i) {
        int status = pthread_mutex_destroy(&mutexes[i]);
        if (status != LAB_NO_ERROR) fatal = status;
    }
    return fatal;
}

errorIndexPair initMutexes(pthread_mutex_t *mutexes, long n) {
    pthread_mutexattr_t attr;
    pthread_mutexattr_init(&attr);
    
    int status = pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK);
    if (status != LAB_NO_ERROR) return (errorIndexPair){status, 0};

    for (int i = 0; i < LAB_MUTEX_NUMBER; ++i) {
        status = pthread_mutex_init(&mutexes[i], &attr);
        if (status != LAB_NO_ERROR) return (errorIndexPair){status, i};
    }

    return (errorIndexPair){LAB_NO_ERROR, n - 1};
}

threadLabNode *checkResults(threadLabNode *threads, long n) {
    for (long i = 0; i < n; ++i) {
        int status = threads[i].status;
        if (status != LAB_NO_ERROR) return &threads[i];
    }

    return NULL;
}

void describeSectionAndError(int section, int status, pthread_t thread) {
    switch (section)
    {
    case LAB_LOCK_SECTION:
        printError(status, thread, "problem in aquiring lock");
        break;
    case LAB_UNLOCK_SECTION:
        printError(status, thread, "problem in unlocking");
        break;
    case LAB_PSEUDOSYNC_SECTION:
        printError(status, thread, "problem in pseudo-sync");
        break;
    case LAB_END_SECTION:
        printError(status, thread, "can't unlock print-mutex");
        break;
    default:
        printf("shouldn't reach there\n");
        exit(LAB_FATAL);
    }
}

void runChildrenThreads(long iterations) {
    pthread_mutex_t mutexes[LAB_MUTEX_NUMBER];
    threadLabNode threads[LAB_THREADS_NUMBER];
    
    errorIndexPair result = initMutexes(mutexes, LAB_MUTEX_NUMBER);
    if (result.status != LAB_NO_ERROR) {
        printError(result.status, pthread_self(), result.i == 0 ? "can't init mutex attributes" :  "can't init mutexes"); 
        deinitMutexes(mutexes, result.i);
        exit(LAB_CANT_INIT_MUTEX);
    }

    initThreads(mutexes, threads, LAB_THREADS_NUMBER, iterations);
    threadLabNode * problem = runThreads(threads, LAB_THREADS_NUMBER);
    if (problem != NULL) {
        printError(problem->status, problem->thread, "thread creation problem, calling exit");
        deinitMutexes(mutexes, result.i);
        exit(LAB_CANT_CREATE_THREADS);
    } 

    problem = waitUntilAllThreadsFinish(threads, LAB_THREADS_NUMBER);
    if (problem != NULL) {
        printError(problem->status, problem->thread, "couldn't wait for this thread due to some error");
        deinitMutexes(mutexes, result.i);
        exit(LAB_CANT_WAIT_FOR_THREADS);
    }

    problem = checkResults(threads, LAB_THREADS_NUMBER);
    if (problem != NULL) {
        describeSectionAndError(problem->section, problem->status, problem->thread);
        deinitMutexes(mutexes, result.i);
        exit(LAB_BAD);
    }

    if (deinitMutexes(mutexes, LAB_MUTEX_NUMBER) != LAB_NO_ERROR) 
        exit(LAB_FATAL);
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
        printf("bad input: too long or start with 0\n");
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
    runChildrenThreads(iterations);
    exit(LAB_NO_ERROR);
}
