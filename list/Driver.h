#ifndef _DRIVER_HPP
#define _DRIVER_HPP
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <sched.h>
#include <poll.h>
#include <alloca.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "plat.h"
#include "timeWrapper.h"
#include "Set.h"

using namespace std;

#define DEFAULT_NUMOPS 100000
#define DEFAULT_RANGE 10000
#define DEFAULT_MUT_PCNT 10
#define INITIAL_ALLOC (64 * 1024)

typedef struct thread_arg {
  int threadNum;
  Set* set;
  int range;
  int numOps;
  int mutPcnt;
  volatile int balance;
  int* balanceArr;
  volatile hrtime_t runTime;
  mt19937 rand;
  pthread_barrier_t* barrier;
  ThreadLocal* threadLocal;
  thread_arg() :
    set(NULL),
    balance(0),
    balanceArr(NULL),
    rand(time(0))
  {}
} thread_arg_t;

// The HEADER macro defines the header file name: HEADER.h
// These macros stringify header file name
#define xstr(x) str(x.h)
#define str(x) #x
#include xstr(HEADER)

#endif /* _DRIVER_HPP */
