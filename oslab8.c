#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <limits.h>

#define LAB_NO_ERROR 0
#define LAB_SOME_ERROR 1

#define LAB_CANT_CREATE_THREADS 2
#define LAB_THREAD_NOT_STARTED 3
#define LAB_CANT_WAIT_FOR_THREADS 4
#define LAB_BAD_ARGS 5
#define LAB_BAD_ALLOC 6

#define LAB_ITERATION_NUMBER 100000000
#define LAB_MAX_THREADS_NUMBER 32

// #define LAB_DEBUG

// typedef unsigned int pthread_t;
// int pthread_create(pthread_t *thr, void * p,  void *(*start_routine)(void*), void * arg);\
// int pthread_join(pthread_t thread, void **status);
// void pthread_exit(void *value_ptr);

typedef struct _threadRunParams {
    unsigned long startIndex;
    unsigned long count;
    long iterationsNumber;
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

double LeibnizPi(double i) {
    double f = floor(i);
    return pow(-1, f) / (2*i + 1.0);
}

void * run(void * param) {
    if (param == NULL)
        return param;
    
    threadLabNode * tn = (threadLabNode*)param;
    runParams p = tn->params;
    
    double res = 0;
    double x = p.startIndex;
    
    for (long i = 0; i < p.iterationsNumber; ++i) {
        res += LeibnizPi(x);
        x += p.count;
    }
#ifdef LAB_DEBUG
    printf("%d %.15g\n", p.startIndex, 4 * res);
#endif
    tn->params.result = res;
    return param;
}

void printError(int code, pthread_t thread, char * what) {
    fprintf(stderr, "Error with thr %lu\n%s; %s\n", thread, what, strerror(code));
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

double collectResults(threadLabNode *finishedThreads, long n) {
    double res = 0;
    for (long i = 0; i < n; ++i) res += finishedThreads[i].params.result;
    return res;
}

void initThreads(threadLabNode *threads, long n, long iterations) {
    for (long i = 0; i < n; ++i) {
        runParams params = {(unsigned long)i, (unsigned long)n, iterations, 0.0};
        threads[i] = constructNode(params);
    }
}

int isCorrect(long v, char * rep) {
    char ns[64];
    sprintf(ns, "%ld", v);
    return strcmp(ns, rep) == 0;
}

void initAndMayBeDie(int argc, char *argv[], long *n, long *iterations) { //simplify next 
    if (argc < 2) {
        printf("args: threadsNumber [ iterationsNumber ]\n");
        exit(LAB_BAD_ARGS);
    }
    *n = strtol(argv[1], (char **)NULL, 10);
    if ((*n) > LAB_MAX_THREADS_NUMBER) {
        printf("Max threads number is: %d\n", LAB_MAX_THREADS_NUMBER);
        exit(LAB_BAD_ARGS);
    }

    if (isCorrect(*n, argv[1]) != 1) {
        printf("threadsNumber may be contains not only digits\n");
        exit(LAB_BAD_ARGS);
    } else if (errno) {
        printError(errno, pthread_self(), "can't read number of threads");
        exit(LAB_BAD_ARGS);
    } else if (*n <= 0) {
        printf("threadsNumber must be positive\n");
        exit(LAB_BAD_ARGS);
    }

    if (argc < 3) {
        *iterations = LAB_ITERATION_NUMBER;
        return;
    }
    *iterations = strtol(argv[2], (char**)NULL, 10);
    if (isCorrect(*iterations, argv[2]) != 1) {
        printf("iterationsNumber must contain only digits and mustn't start with 0\n");
        exit(LAB_BAD_ARGS);
    } else if (*iterations <= 0) {
        printf("iterationsNumber must be positive\n");
        exit(LAB_BAD_ARGS);
    } else if (errno) {
        printError(errno, pthread_self(), "can't read number of iterations");
        exit(LAB_BAD_ARGS);
    }
    // if (((*iterations) * (*n)) / (*n) != (*iterations)) {
    //     printf("Summary number of sequence terms is too big (iterations * n)\n We recommend lower number of iterations\n");
    //     exit(LAB_BAD_ARGS);
    // }
}

double runMultiThreadCalculations(long n, long iterations) {
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

    double pi = 4.0 * collectResults(threads, n);
    return pi;
}

int main(int argc, char *argv[]) {
    long n;
    long iterations;
    initAndMayBeDie(argc, argv, &n, &iterations);
    
    double pi = runMultiThreadCalculations(n, iterations);    
    printf("pi=%.30g\n", pi);

    exit(LAB_NO_ERROR);
}
