#include <stdio.h>
#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include <sched.h>

#include <signal.h>

#define LAB_NO_ERROR 0
#define LAB_SOME_ERROR 1

#define LAB_CANT_CREATE_THREADS 2
#define LAB_THREAD_NOT_STARTED 3
#define LAB_CANT_WAIT_FOR_THREADS 4
#define LAB_BAD_ARGS 5
#define LAB_BAD_ALLOC 6

#define  LAB_RANGE 10000

typedef struct _threadRunParams {
    double start;
    double range;
    long totalThreads;
} runParams;
typedef struct _threadLabNode threadLabNode;
struct _threadLabNode {
    runParams params;
    pthread_t thread;
    double result; 
    int status;
};

threadLabNode constructNode(runParams p) {
    threadLabNode node;
    node.params = p;
    node.status = LAB_NO_ERROR;
    node.result = 0;
    return node;    
}

void printError(int code, pthread_t thread, char * what) {
    fprintf(stderr, "Error with thr %lu\n%s; %s\n", thread, what, strerror(code));
}

double LeibnizPi(double i) {
    double f = floor(i);
    return pow(-1, f) / (2*i + 1.0);
}

static int doRun = 1;
/*
 * this function should not be called from anywhere except signal handler
 * since it affects work of different threads
 */
void sigcatch(int sig) {
    doRun = 0;
}

void blockSIGINT() {
    sigset_t new;
    sigemptyset(&new);
    sigaddset(&new, SIGINT);
    pthread_sigmask(SIG_BLOCK, &new, NULL);
}

void unblockSIGINT() {
    sigset_t new;
    sigemptyset(&new);
    sigaddset(&new, SIGINT);
    pthread_sigmask(SIG_UNBLOCK, &new, NULL);
}

void setSigcatch() {
    struct sigaction act;
    act.sa_handler = sigcatch;    
    //https://illumos.org/man/2/sigaction
    if (sigaction(SIGINT, &act, NULL) == EINVAL) {
        printError(EINVAL, pthread_self(), "can't set signal SIGINT for main thread");
        exit(LAB_SOME_ERROR);
    }
}

void * run(void * param) {
    if (param == NULL)
        return param;
    
    threadLabNode * tn = (threadLabNode*)param;
    runParams p = tn->params;

    double res = 0;
    long range = (long)p.range;
#ifdef LAB_DEBUG
    printf("range: %d\n", range);
#endif
    while (doRun) {
        double x = p.start;
        double subRes = 0;
        for (long i = 0; i < range; ++i) {
            subRes += LeibnizPi(x);
            x++;
        }
        res += subRes;
        p.start += p.range * p.totalThreads;
    }
#ifdef LAB_DEBUG
    double numberOfIterations = p.start;
    printf("%d %.15g %.15g\n", pthread_self(), numberOfIterations,4 * res);
#endif
    tn->result = res;
    return param;
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
        if (status == LAB_NO_ERROR) {
            threadLabNode * ret = NULL;
            int code = pthread_join(curr->thread, (void**)(&ret));
            curr->status = code;
            if (code != LAB_NO_ERROR)
                return curr;
        } else {
            /*Means thread wasn't started or already was in this *good* branch*/
        }
    }

    return NULL;
}

double collectResults(threadLabNode *finishedThreads, long n) {
    double res = 0;
    for (long i = 0; i < n; ++i) res += finishedThreads[i].result;
    return res;
}

void initThreads(threadLabNode *threads, long n, double range) {
    for (long i = 0; i < n; ++i) {
        runParams params = {range*i, range, n};
        threads[i] = constructNode(params);
    }
}

void initAndMayBeDie(int argc, char *argv[], long *n) {
    if (argc < 2) {
        printf("args: threadsNumber\n");
        exit(LAB_BAD_ARGS);
    }

    *n = strtol(argv[1], (char **)NULL, 10);

    if (errno) {
        printError(errno, pthread_self(), "can't read number of threads");
        exit(LAB_BAD_ARGS);
    }
}

double runMultiThreadCalculations(long n) {
    threadLabNode *threads = malloc(sizeof(threadLabNode) * n);
    if (threads == NULL) {
        printError(ENOMEM, pthread_self(), "threads number is too big");
        exit(LAB_CANT_CREATE_THREADS);
    }
    
    initThreads(threads, n, LAB_RANGE);
    
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

    free(threads);
    return 4.0 * collectResults(threads, n);
}

int main(int argc, char *argv[]) {
    setSigcatch();
    
    long n;
    initAndMayBeDie(argc, argv, &n);
    
    double pi = runMultiThreadCalculations(n);    
    printf("pi=%.30g\n", pi);

    exit(LAB_NO_ERROR);
}
