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

static int64_t g_count = 0;
static int g_pushRunning = 1;
static int g_popRunning = 1;

void* PushEntry(void* arg)
{
    prctl(PR_SET_NAME, "push");

    uint64_t lfq = (uint64_t)(unsigned long)arg;
    while (g_pushRunning)
    {
        Test_t* ptr = (Test_t*)malloc(sizeof(Test_t));
        if (!ptr) continue;
        
        if (lfqPush(lfq, (void*)ptr) != 0)
        {
            free(ptr);
            continue;
        }
        
        __sync_add_and_fetch(&g_count, 1);
    }

    return 0;
}

void* PopEntry(void* arg)
{
    prctl(PR_SET_NAME, "pop");

    uint64_t lfq = (uint64_t)(unsigned long)arg;
    while (g_popRunning)
    {
        Test_t* ptr = 0;
        if (lfqPop(lfq, (void**)&ptr) == 0)
        {
            free(ptr);
            __sync_sub_and_fetch(g_count, 1);
        }
    }

    return 0;
}

void main(int argc, char** argv)
{
    if (argc < 7)
    {
        printf("usage: %s queueCount queueSize pushThreadCount popThreadCount pushSeconds popSeconds \n", argv[0]);
        return;
    }
    
    int queueCnt = atoi(argv[1]);
    int queueSize = atoi(argv[2]);
    int pushThCnt = atoi(argv[3]);
    int popThCnt = atoi(argv[4]);
    int pushSeconds = atoi(argv[5]);
    int popSeconds = atoi(argv[6]);
    
    for (int k = 0; k < queueCnt; k++)
    {
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
    }
    
    printf("%d threads pushing and %d threads popping ... \n", pushThCnt, popThCnt);
    sleep(pushSeconds);
    
    g_pushRunning = 0;
    printf("%d threads popping ... \n", popThCnt);
    sleep(popSeconds - pushSeconds);
    
    g_popRunning = 0;
    sleep(1);
    
    if (g_count == 0) printf("INFO, test success, pushCount - popCount = %lld \n", g_count);
    else printf("ERROR, test failed, pushCount - popCount = %lld \n", g_count);    
    return;
}
