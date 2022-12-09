#include "LockFreeQueue.h"
#include <stdio.h>
#include <errno.h>
#include <malloc.h>

#define LOG_LOGGER(level, fmt, ...) printf("%s | %s: %s(%d) | " fmt "\n", level, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS__);
#define LOG_DEBUG(fmt, ...) LOG_LOGGER("debug", fmt, ##__VA_ARGS__);
#define LOG_INFO(fmt, ...) LOG_LOGGER("info", fmt, ##__VA_ARGS__);
#define LOG_WARN(fmt, ...) LOG_LOGGER("warn", fmt, ##__VA_ARGS__);
#define LOG_ERROR(fmt, ...) LOG_LOGGER("error", fmt, ##__VA_ARGS__);
#define LOG_FATAL(fmt, ...) LOG_LOGGER("fatal", fmt, ##__VA_ARGS__);

typedef struct tagLockFreeElement
{
    struct tagLockFreeElement* next;
    void* data;
    uint16_t count;
    uint8_t using;
} LockFreeElement_t;

typedef struct tagLockFreeQueue
{
    LockFreeElement_t* head;
    LockFreeElement_t* tail;
    uint32_t size;
    LockFreeElement_t array[1];
} LockFreeQueue_t;

#define ADD_HDR(ptr, count) ((LockFreeElement_t*)((uint64_t)(unsigned long)ptr | ((uint64_t)count) << 48))
#define CUT_HDR(ptr) ((LockFreeElement_t*)((uint64_t)(unsigned long)ptr & 0xFFFFFFFFFFFFULL))

static void freeElement(LockFreeElement_t* e)
{
    e = CUT_HDR(e);
    e->next = 0;
    e->data = 0;
    e->using = 0;
}

static LockFreeElement_t* mallocElement(LockFreeQueue_t* lfq)
{
    for (uint32_t i = 0; i < lfq->size; i++)
    {
        LockFreeElement_t* e = &lfq->array[i];
        uint8_t using = e->using;
        if (using) continue;
        if (!__sync_bool_compare_and_swap(&e->using, using, (uint8_t)1)) continue;       
        e->next = 0;
        e->data = 0;
        e->count = (e->count + 1) % 0xFFFF;
        return ADD_HDR(e, e->count);
    }
    
    return 0;
}

uint64_t lfqCreate(uint32_t size)
{
    if (size == 0)
    {
        LOG_ERROR("size == 0");
        return 0;
    }

    uint32_t allSize = sizeof(LockFreeQueue_t) + (size - 1) * sizeof(LockFreeElement_t);
    LockFreeQueue_t* lfq = (LockFreeQueue_t*) malloc(allSize);
    if (lfq == 0)
    {
        LOG_ERROR("malloc(%u) failed, errno[%d]", allSize, errno);
        return 0;
    }

    lfq->size = size;
    for (uint32_t i = 0; i < size; i++)
    {
        lfq->array[i].next = 0;    
        lfq->array[i].data = 0;
        lfq->array[i].count = 0;
        lfq->array[i].using = 0;
    }
    lfq->head = lfq->tail = mallocElement(lfq);
    
    LOG_DEBUG("create LockFreeQueue with size[%u] success", size);
    return (uint64_t)(unsigned long)lfq;
}

int lfqPush(uint64_t handle, void* data)
{
    if (data == 0)
    {
        LOG_ERROR("data == 0");
        return -1;
    }
    
    LockFreeQueue_t* lfq = (LockFreeQueue_t*)(unsigned long)handle;
    if (lfq == 0)
    {
        LOG_ERROR("handle == 0");
        return -1;
    }

    LockFreeElement_t* e = mallocElement(lfq);
    if (!e)
    {
        LOG_ERROR("queue full");
        return -1;
    }
    CUT_HDR(e)->data = data;
    
    LockFreeElement_t* tail = 0;
    LockFreeElement_t* next = 0;
    while(1)
    {
        // seek tail
        tail = lfq->tail;
        next = CUT_HDR(tail)->next;
        if (next)
        {
            __sync_bool_compare_and_swap(&lfq->tail, tail, next);
            continue;
        }
        
        // push tail
        if (__sync_bool_compare_and_swap(&CUT_HDR(tail)->next, 0, e))
        {
            break;
        }
    }

    while(1)
    {
        // seek tail
        tail = lfq->tail;
        next = CUT_HDR(tail)->next;
        if (next)
        {
            __sync_bool_compare_and_swap(&lfq->tail, tail, next);
            continue;
        }
        break;
    }

    return 0;
}

int lfqPop(uint64_t handle, void** data)
{
    if (data == 0)
    {
        LOG_ERROR("data == 0");
        return -1;
    }
    *data = 0;
    
    LockFreeQueue_t* lfq = (LockFreeQueue_t*)(unsigned long)handle;
    if (lfq == 0)
    {
        LOG_ERROR("handle == 0");
        return -1;
    }

    void* tmpData = 0;
    LockFreeElement_t* head = 0;
    LockFreeElement_t* tail = 0;
    LockFreeElement_t* next = 0;
    while (1)
    {
        // seek tail
        tail = lfq->tail;
        next = CUT_HDR(tail)->next;
        if (next)
        {
            __sync_bool_compare_and_swap(&lfq->tail, tail, next);
            continue;
        }
        
        // check empty
        head = lfq->head;
        next = CUT_HDR(head)->next;
        if (!next)
        {
            LOG_ERROR("queue empty");
            return -1;
        }
        
        // pop head
        tmpData = CUT_HDR(next)->data;
        if (__sync_bool_compare_and_swap(&lfq->head, head, next))
        {
            break;
        }
    }

    *data = tmpData;
    freeElement(head);

    while(1)
    {
        // seek tail
        tail = lfq->tail;
        next = CUT_HDR(tail)->next;
        if (next)
        {
            __sync_bool_compare_and_swap(&lfq->tail, tail, next);
            continue;
        }
        break;
    }
    
    return 0;
}

