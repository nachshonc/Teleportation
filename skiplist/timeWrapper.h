#ifndef _TIME_USR_H
#define _TIME_USR_H

#if defined(__linux)

#include <time.h>
#include <sched.h>
#include <linux/unistd.h>
#include <sys/syscall.h>

typedef int64_t hrtime_t;

// Emulate Solaris gethrtime() on Linux
//
// Possible user-mode software clock sources :
// * CLOCK_MONOTONIC
//   Faster than CLOCK_MONOTONIC_RAW on a 3.6 kernel.
//   Implemented via hpet or RDTSC/RDTSCP via a VDSO fast-path
//   On a warmed-up 3.0Ghz Nehalem i5 with a 3.6 kernel the round-trip for
//   CLOCK_MONOTONIC is about 25 nsecs
// * CLOCK_MONOTONIC_RAW;
//   85 nsec round-trip latency
// * CLOCK_MONOTONIC_HR;
// * CLOCK_MONOTONIC_COARSE
//   Implemented via VDSO fast-path
//   CLOCK_MONOTONIC_COARSE is very fast but simply reads lbolt/jiffies
//   under a seqlock.  It should provide causal consistency on an MP.
//   On the system-under-test it advances every 400 usecs : 250Hz
// * Raw user-mode access to RDTSC and RDTSCP
//
// See /sys/devices/system/clocksource0/current_clocksource and available_clocksource
// The main choices are "tsc" and "hpet"
//
// Note that getcpu() is implemented as an VDSO fast-call via RDTSCP

static clockid_t BaseClock = CLOCK_MONOTONIC ;
static hrtime_t gethrtime() {
  static volatile int BaseEpoch = 0 ;
  struct timespec tp ;
  tp.tv_sec  = 0 ;
  tp.tv_nsec = 0 ;
  clock_gettime (CLOCK_MONOTONIC, &tp) ;
  if (BaseEpoch == 0) {
    BaseEpoch = tp.tv_sec ;
  }
  // We assume the result is normalized
  // Beware of racy initialization
  // We assume main() will call gethrtime() early, while single-threaded, to
  // initialize BaseEpoch
  tp.tv_sec -= BaseEpoch ;

  // Consider using the fast multiply-by-1B shift-add idiom from
  // the solaris kernel.
  return (tp.tv_sec * 1000000000LL) + tp.tv_nsec ;
}

#else

#include <sys/time.h>

#endif // defined(__linux)

#endif //_TIME_USR_H

