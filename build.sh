#!/bin/bash

gcc LockFreeQueue.c -shared -fPIC -g3 -O0 -std=c99 -I. -o libLockFreeQueue.so
gcc test.c -fPIE -g3 -O0 -std=c99 -I. -L. -lLockFreeQueue -lpthread -o test

exit 0

