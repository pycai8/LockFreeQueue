#include "LockFreeQueue.h"

#define LOG_LOGGER(level, fmt, ...) printf("%s | %s: %s(%d) | " fmt, level, __FILE__, __FUNCTION__, __LINE__, ##__VA_ARGS);
#define LOG_DEBUG(fmt, ...) LOG_LOGGER("debug", fmt, ##__VA_ARGS__);
#define LOG_INFO(fmt, ...) LOG_LOGGER("info", fmt, ##__VA_ARGS__);
#define LOG_WARN(fmt, ...) LOG_LOGGER("warn", fmt, ##__VA_ARGS__);
#define LOG_ERROR(fmt, ...) LOG_LOGGER("error", fmt, ##__VA_ARGS__);
#define LOG_FATAL(fmt, ...) LOG_LOGGER("fatal", fmt, ##__VA_ARGS__);

typedef struct LockFreeElement_t
{
    LockFreeElement_t* next;
    void* data;
    uint8_t using;
} LockFreeElement_t;

typedef struct
{
    LockFreeElement_t* head;
    LockFreeElement_t* tail;
    uint32_t size;
    LockFreeElement_t array[1];
} LockFreeQueue_t;

static void freeElement(LockFreeElement_t* e)
{
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
        if (!__sync_bool_compare_and_swap(e->using, 0, 1)) continue;       
        e->next = 0;
        e->data = 0;
        return e;
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
        lfq->array[i].using = 0;
        lfq->array[i].data = 0;
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
    e->data = data;
    
    while(1)
    {
        // seek tail
        LockFreeElement_t* tail = lfq->tail;
        LockFreeElement_t* next = tail->next;
        if (next)
        {
            __sync_bool_compare_and_swap(&lfq->tail, tail, next);
            continue;
        }
        
        // push tail
        if (__sync_bool_compare_and_swap(&tail->next, 0, e))
        {
            return 0;
        }
    }

	LOG_ERROR("this is impossible");
	return -1;
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

    LockFreeElement_t* array = lfq->array;
	while (1)
	{
        // seek tail
        LockFreeElement_t* tail = lfq->tail;
        LockFreeElement_t* next = tail->next;
        if (next)
        {
            __sync_bool_compare_and_swap(&lfq->tail, tail, next);
            continue;
        }
        
        // check empty
        LockFreeElement_t* head = lfq->head;
        next = head->next;
        if (!next)
        {
            LOG_ERROR("queue empty");
            return -1;
        }
        
        // pop head
        void* tmpData = next->data;
        if (__sync_bool_compare_and_swap(&lfq->head, head, next))
        {
            freeElement(head);
            *data = tmpData;
            return 0;
        }
	}

	LOG_ERROR("this is impossible");
	return -1;
}

