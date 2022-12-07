#include <stdio.h>
#include <malloc.h>
#include <pthread.h>
#include <stdlib.h>
#include <sys/unistd.h>
#include <sys/prctl.h>
#include "LockFreeQueue.h"

typedef struct
{
    int a;
    long b;
    char c[10];
} Test_t;

void* PushEntry(void* arg)
{
    prctl(PR_SET_NAME, "push");

    uint64_t lfq = (uint64_t)(unsigned long)arg;
    while (1)
    {
        Test_t* ptr = (Test_t*)malloc(sizeof(Test_t));
        if (!ptr) continue;
        
        if (lfqPush(lfq, (void*)ptr) != 0)
        {
            free(ptr);
        }
    }

    return 0;
}

void* PopEntry(void* arg)
{
    prctl(PR_SET_NAME, "pop");

    uint64_t lfq = (uint64_t)(unsigned long)arg;
    while (10)
    {
        Test_t* ptr = 0;
        if (lfqPop(lfq, (void**)&ptr) == 0)
        {
            free(ptr);
        }
    }

    return 0;
}

void main(int argc, char** argv)
{
    if (argc < 4)
    {
        printf("usage: %s queueSize pushThreadCount popThreadCount\n", argv[0]);
        return;
    }
    
    int queueSize = atoi(argv[1]);
    int pushThCnt = atoi(argv[2]);
    int popThCnt = atoi(argv[3]);
    
    uint64_t lfq = lfqCreate(queueSize);
    if (lfq == 0)
    {
        printf("create lock free queue of size[%d] failed\n", queueSize);
        return;
    }
    
    for (int i = 0; i < pushThCnt; i++)
    {
        pthread_t th = 0;
        pthread_create(&th, 0, PushEntry, (void*)(unsigned long)lfq);
    }
    for (int i = 0; i < popThCnt; i++)
    {
        pthread_t th = 0;
        pthread_create(&th, 0, PopEntry, (void*)(unsigned long)lfq);
    }
    
    printf("running ... \n");
    while (1) sleep(3600);
    return;
}
