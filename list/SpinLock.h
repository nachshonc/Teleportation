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

/* Standard Oracle types */
typedef unsigned char ub1;
typedef unsigned short ub2;
typedef unsigned int ub4;
typedef unsigned long long ub8;
#ifdef HAND_OVER_HAND
#define _ALIGN(x)    __attribute__ ((aligned(x)))
#else
#define _ALIGN(x)
#endif

using namespace std;
/*
 * SpinLock taken from fastsync code
 */
class SpinLock {
public:
  static const ub4 UNLOCKED = 0xdeadbeef;
  static const ub4 LOCKED = 0xcafebabe;
  //ub4 align1[15]; // make each lock 128 bytes to ensure separate cache line
  volatile ub4 val ;// the current lock value
  //ub4 align2[16]; // make each lock 128 bytes to ensure separate cache line

#if defined(DEBUG)
protected:
  char buf[512];
#endif

public:
  SpinLock():val(UNLOCKED) {
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
      ub4 result = CAS32 (&val, UNLOCKED, LOCKED);
      if (result == UNLOCKED) {
	return;
      } else {
	while (true) {
	  while (val != UNLOCKED) {
	    Pause() ;
	  }
	  result = CAS32 (&val, UNLOCKED, LOCKED);
	  if (result == UNLOCKED) {
	    return;
	  }
	}
      }
    }
  }

  inline bool isLocked() {
    return val == LOCKED;
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
    __asm__ __volatile__ ("lock;addl $0,(%%rsp);" ::: "cc","memory") ;
  };
  inline void cfence(){ __asm__ __volatile__ ("":::"memory");}

  inline void unlock() {
    // Constraint and requirement : &val must be 64B aligned to use BIS !
    // XXX MEMBAR(StoreStore) ; _BIS (UNS(&val) & ~0x3F) ;
    val = UNLOCKED;
    cfence();
    //membar();
  }

  char* toString(char* buf) {
    if (val == UNLOCKED) {
      sprintf(buf, "%p->UNLOCkED", this);
    } else if (val == LOCKED) {
      sprintf(buf, "%p->LOCKED", this);
    } else {
      sprintf(buf, "%p->UNKNOWN: %x", this, val);
    }
    return buf;
  }

} _ALIGN(64) ;
#endif	/* _SPINLOCK_H */
