#!/bin/bash

gcc LockFreeQueue.c -shared -fPIC -g3 -O0 -I. -o libLockFreeQueue.so

exit 0

