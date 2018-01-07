#ifndef _SPINLOCK_H
#define	_SPINLOCK_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
//#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <sched.h>
#include <poll.h>
#include <alloca.h>
#include <string.h>
#include <math.h>
#include "xabort.h"
#include <immintrin.h>
#include <rtmintrin.h>
#include <xtestintrin.h>


/* Standard Oracle types */
typedef unsigned char ub1;
typedef unsigned short ub2;
typedef unsigned int ub4;
typedef unsigned long long ub8;
#define _ALIGN(x)    __attribute__ ((aligned(x)))
#define Pause pthread_yield
#define CAS64 __sync_bool_compare_and_swap

using namespace std;
/*
 * SpinLock taken from fastsync code
 */
class SpinLock {
  static const int UNLOCKED = 0;
  static const int LOCKED = 1;
  ub4 align1[15]; // make each lock 128 bytes to ensure separate cache line
  volatile ub4 val _ALIGN(64) ; // the current lock value
  ub4 align2[16]; // make each lock 128 bytes to ensure separate cache line

public:
  SpinLock() {
    val = UNLOCKED ; 
  }

  inline void lock() {
    // in transaction?
    if (_xtest()) {
      if (val != UNLOCKED) {
	_xabort(XABORT_LOCKED);
      } else {
	val = LOCKED;
      }
    } else {
      ub4 result = CAS64 (&val, UNLOCKED, LOCKED);
      if (result == UNLOCKED) {
	return;
      } else {
	while (true) {
	  while (val != UNLOCKED) {
	    Pause() ;
	  }
	  result = CAS64 (&val, UNLOCKED, LOCKED);
	  if (result == UNLOCKED) {
	    return;
	  }
	}
      }
    }
  }

  inline bool isLocked() {
    return val != UNLOCKED;
  }
 
  // Fibonacci back-off 
  // Derived from HotSpot JVM synchronizer.cpp
  // See also 1BL-Spin1F
  //   inline bool _FIB_lock() {
  //     if (cas_ub4 (&val, 0, 1)) return false ; 
  //     int fa = 0 ;
  //     int fb = 1 ; 
  //     for (;;) { 
  //       int penalty = 0 ; 
  //       if (val == 0) { 
  //         if (cas_ub4 (&val, 0, 1)) return true ; 
  //         penalty = 100 ; 
  //       }
  //       fb += fa ; 
  //       fa = fb-fa ; 
  //       if (fb > 200) fb = 200 ; 
  //       for (int k = fb + penalty ; --k >= 0 ; ) { 
  //         Pause() ; 
  //       }
  //     }
  //   }

  inline void membar() {
    __asm__ __volatile__ ("lock;addl $0,(%%rsp);" ::: "cc") ;
  };

  inline void unlock() {
    // Constraint and requirement : &val must be 64B aligned to use BIS !
    // XXX MEMBAR(StoreStore) ; _BIS (UNS(&val) & ~0x3F) ;
    val = UNLOCKED;
    membar();
  }

  inline void print() {
    printf("SpinLock[%x]", val);
  }

} _ALIGN(64) ; 
#endif	/* _SPINLOCK_H */
