#include "LockFreeQueue.h"

#define NULL_ELEMENT 0xffffffffU

typedef struct
{
    uint32_t next;
    uint32_t using;
    uint64_t data;
} LockFreeElement_t;

typedef struct
{
    uint32_t head;
    uint32_t tail;
    uint32_t size;
    LockFreeElement_t array[1];
} LockFreeQueue_t;

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
        lfq->array[i].next = NULL_ELEMENT;
        lfq->array[i].using = 0;
        lfq->array[i].data = 0;
    }
    lfq->head = lfq->tail = mallocElement(lfq);
    
	LOG_DEBUG("create LockFreeQueue with size[%u] success", size);
	return (uint64_t)(long)lfq;
}

int lfqPush(uint64_t handle, void* data)
{
	LockFreeQueue_t* lfq = (LockFreeQueue_t*)handle;
	if (lfq == 0)
	{
		LOG_ERROR("handle == 0");
		return -1;
	}

	if (data == 0)
	{
		LOG_ERROR("data == 0");
		return -1;
	}

    uint32_t element = mallocElement(lfq);
    if (element == NULL_ELEMENT)
    {
        LOG_ERROR("queue full");
        return -1;
    }
    lfq->array[element].data = (uint64_t)(long)data;
    
    uint32_t tail = lfq->tail;
    LockFreeElement_t* array = lfq->array;
	while (1)
	{
        uint64_t tmp = __sync_val_compare_and_swap(&array[tail].data, 0, (uint64_t)(long)data);
        if (tmp == 0)
        {
            LOG_DEBUG("lfqPush success");
            freeElement(element);
            return 0;
        }
        
        tail = __sync_val_compare_and_swap(&array[tail].next, NULL_ELEMENT, element);
        if (tail == NULL_ELEMENT)
        {
            LOG_DEBUG("lfqPush success");
            return 0;
        }
	}

	LOG_ERROR("this is impossible");
	return -1;
}

int lfqPop(uint64_t handle, void** data)
{
	LockFreeQueue_t* lfq = (LockFreeQueue_t*)handle;
	if (lfq == 0)
	{
		LOG_ERROR("handle == 0");
		return -1;
	}

	if (data == 0)
	{
		LOG_ERROR("data == 0");
		return -1;
	}
	*data = 0;

    LockFreeElement_t* array = lfq->array;
	while (1)
	{
        uint32_t head = lfq->head;
        void* ret = array[head].data;
        uint32_t next = array[head].next;
        uint32_t tail = lfq->tail;
    
        if (head == tail)
        {
            LOG_ERROR("queue empty");
            return -1;
        }
        
        uint32_t tmp = __sync_val_compare_and_swap(&lfq->head, head, next);
        if (tmp == head)
        {
            freeElement(head);
            *data = ret;
            return 0;
        }
	}

	LOG_ERROR("this is impossible");
	return -1;
}

