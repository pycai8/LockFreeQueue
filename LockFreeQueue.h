#ifndef __LOCK_FREE_QUEUE_H__
#define __LOCK_FREE_QUEUE_H__

#include <stdint.h>

uint64_t lfqCreate(uint32_t size);

int lfqPush(uint64_t handle, void* data);

int lfqPop(uint64_t handle, void** data);

#endif


