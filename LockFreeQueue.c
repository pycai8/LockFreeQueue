#include "LockFreeQueue.h"

typedef struct
{
	uint32_t size;
	uint64_t startAndCount; // startIndex 4Byte, Count 4Byte
	void* array[1];
} LockFreeQueue_t;

uint64_t lfqCreate(uint32_t size)
{
	if (size == 0)
	{
		LOG_ERROR("size == 0");
		return 0;
	}

	uint32_t allSize = sizeof(LockFreeQueue_t) + (size - 1) * sizeof(void*);
	LockFreeQueue_t* lfq = (LockFreeQueue_t*) malloc(sizeof(LockFreeQueue_t) + (size - 1) * sizeof(void*));
	if (lfq == 0)
	{
		LOG_ERROR("malloc(%u) failed, errno[%d]", allSize, errno);
		return 0;
	}

	lfq->size = size;
	lfq->startAndCount = 0;
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

	while (1)
	{
		uint64_t oldStartCount = lfq->startAndCount;
		uint64_t start = oldStartCount >> 32;
		uint64_t count = oldStartCount & 0xffffffffULL;
		if (count == lfq->size)
		{
			LOG_ERROR("queue full");
			return -1;
		}

		count++;
		uint64_t index = (start + count - 1) % lfq->size;
		uint64_t newStartCount = (start << 32 | count);
		if (__sync_bool_compare_and_swap(&lfq->startAndCount, oldStartCount, newStartCount))
		{
			lfq->array[index] = data;
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

	while (1)
	{
		uint64_t oldStartCount = lfq->startAndCount;
		uint64_t start = oldStartCount >> 32;
		void* tmpData = lfq->array[start];
		uint64_t count = oldStartCount & 0xffffffffULL;
		if (count == 0)
		{
			LOG_ERROR("queue empty");
			return -1;
		}

		uint64_t index = start;
		count--;
		start = (start + 1) % lfq->size;
		uint64_t newStartCount = (start << 32 | count);
		if (__sync_bool_compare_and_swap(&lfq->startAndCount, oldStartCount, newStartCount))
		{
			*data = tmpData;
			LOG_DEBUG("lfqPop success");
			return 0;
		}
	}

	LOG_ERROR("this is impossible");
	return -1;
}

