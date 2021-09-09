#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

void * run(void * param) {
    int i;
    for(i=0; i<10; i++) 
        printf("I was born!\n");
	return NULL;
}

int main(int argc, char *argv[]) {
    // since @param attr is null, default attributes for child are set up using pthread_attr_init(3C)
    // https://illumos.org/man/3C/pthread_create
    // https://illumos.org/man/3C/pthread_attr_init

    pthread_t thread;
    // next 3 lines of code are equal to: 
    // int code = pthread_create(&thread, NULL, run, NULL);
    pthread_attr_t attr;
    pthread_attr_init(&attr); 
    int code = pthread_create(&thread, &attr, run, NULL);
    if (code!=0) {
        char buf[256];
        strerror_r(code, buf, sizeof buf);
        fprintf(stderr, "%s: creating thread: %s\n", argv[0], buf);
        exit(1);
    }

    void * result;
    code = pthread_join(thread, &result);
    if (code!=0) {
        char buf[256];
        strerror_r(code, buf, sizeof buf);
        fprintf(stderr, "%s: creating thread: %s\n", argv[0], buf);
        exit(1);
    }
    
    for(int i=0; i<10; i++) 
        printf("I gave a birth!\n");
    
    // https://illumos.org/man/3C/pthread_attr_init
    pthread_attr_destroy(&attr);
    // https://illumos.org/man/3C/pthread_exit
    pthread_exit(NULL);  
    return 0;
}
