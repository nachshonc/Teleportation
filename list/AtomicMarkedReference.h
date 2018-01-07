#ifndef _ATOMICMARKEDREFERENCE_H
#define _ATOMICMARKEDREFERENCE_H
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "plat.h"
#include "ThreadLocal.h"

class AtomicMarkedReference {

public:
  volatile uintptr_t value;

  AtomicMarkedReference(void* node, bool mark): value(combine(node, mark))
  {  };

  volatile void* set(void* node, bool mark) {
    value = combine(node, mark);
  }

  volatile void* get(bool* mark) {
    uintptr_t current = value;	// atomic read
    *mark = getMark(current);
    return getPtr(current);
  };

  volatile bool isMarked() {
    uintptr_t current = value;	// atomic read
    return getMark(current);
  };

  volatile void* getReference() {
    uintptr_t current = value;	// atomic read
    return getPtr(current);
  };

  volatile bool compareAndSet(void* expectedReference,
			      void* newReference,
			      bool expectedMark,
			      bool newMark) {
    uintptr_t expected = combine(expectedReference, expectedMark);
    uintptr_t replace = combine(newReference, newMark);
    return CAS64(&value, expected, replace) == expected;
  };
  char* toString(char* buf) {
    sprintf(buf,
 	    "AtomicMarkedReference[%p,%d]",
 	    getPtr(value),
 	    getMark(value));
    return buf;
  };

private:
  // bit-masking operations
  inline uintptr_t combine(void* ptr, bool mark) {
    if (mark) {
      return (((uintptr_t) ptr) | ((uintptr_t) 0x1));
    } else {
      return (uintptr_t) ptr;
    }
  }
  inline bool getMark(uintptr_t x) {
    return x & (uintptr_t)0x01;
  }
  inline void* getPtr(uintptr_t x) {
    return (void*) (x & ~(uintptr_t)1);
  }
};

#endif /* _ATOMICMARKEDREFERENCE_H */
