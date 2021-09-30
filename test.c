#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _threadRunParams {
    char **strings;
    int count;
} runParams;

runParams makeStringArray(int n, ...) {
    void * p = &n;
    char ** arr = malloc(sizeof(char*) * n);

    p += sizeof(char*);
    for (int i = 0; i < n; ++i) {
        char * str = *((char**)p);
        arr[i] = strcpy(malloc(strlen(str)), str);
        p += sizeof(char*);
    }
    
    runParams params = {arr, n};
    return params;
}

int main() {
    runParams p = makeStringArray(3, "hi", "s2", "s3");
    return 0;
}