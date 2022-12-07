#!/bin/bash

gcc LockFreeQueue.c -shared -fPIC -g3 -O0 -I. -o libLockFreeQueue.so
gcc test.c -fPIE -g3 -O0 -I. -L. -lLockFreeQueue -o test

exit 0

