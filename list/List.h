/**
 * generic list header
 */
#ifndef _LIST_H
#define _LIST_H
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <algorithm>
#include <iterator>
#include <sched.h>
#include <poll.h>
#include <alloca.h>
#include <string.h>
#include <math.h>
#include "plat.h"
#include "SpinLock.h"
#include "Set.h"
#include "ThreadLocal.h"
#include "AtomicMarkedReference.h"
#define NOINLINE __attribute__ ((noinline))
#define RETRIES 10

#define null NULL

class List: public Set {

#if defined(DEBUG)
protected:
  char buf[512];
#endif
public:
  class Node {
#if defined(DEBUG)
  protected:
    char buf[512];
#endif
  public:
    union{
    AtomicMarkedReference aNext; // used by lock-free
    Node* next;			// used by lazy, hand-over-hnd
    };
    long value;
    int marked;
    Node* free;
    SpinLock spinLock;


    Node(long val): marked(false), value(val),
		    aNext(NULL, false) {
    };
    Node(): aNext(NULL, false),marked(false) {
    };
    Node (long _value, void*  _next): aNext(_next, false),marked(false),
				      value(_value) {
    };
    inline void lock() {
      spinLock.lock();
    }
    inline void unlock() {
      spinLock.unlock();
    }
    inline bool isLocked() {
      return spinLock.isLocked();
    }
    void reset(T v){
      value = v;
      spinLock.unlock();
    }
    char* toString(char* buf) {
      if (value == LONG_MIN) {
	sprintf(buf, "[LSentinel, %s]",
		isLocked() ? "locked" : "unlocked");
      } else if (value == LONG_MAX) {
	sprintf(buf, "[RSentinel, %s]",
		isLocked() ? "locked" : "unlocked");
      } else {
	sprintf(buf, "Node@%p->[%ld,%s]", this,
		value,
		isLocked() ? "locked" : "unlocked");
      }
      return buf;
    }
    inline bool isMarked() {
      return aNext.isMarked();
    }
  };
  static unsigned int randomSeed;
  List():
    LSentinel(LONG_MIN),
    RSentinel(LONG_MAX) {
    LSentinel.next = &RSentinel;
  }

public:
  Node LSentinel;
  Node RSentinel;
  inline void membar() {
    __asm__ __volatile__ ("lock;addl $0,(%%rsp);" ::: "cc") ;
  };
  virtual int size() {		// for validation, not thread-safe
    int result = 0;
    Node* node = LSentinel.next; //NOTE: does not work for lock free, must use aNext. 
    while (node != &RSentinel) {
      if (!node->marked) {
	result++;
      }
      node = node->next;
    }
    return result;
  }

  bool sanity() {		// for validation, not thread-safe
    bool marked = false;	//NOTE: does not work for lock-free, must use aNext. 
    bool locked = false;
    Node* node = LSentinel.next;
    while (node != &RSentinel) {
      if (node->marked) {
	marked = true;
      }
      if (node->isLocked()) {
	locked = true;
      }
      node = node->next;
    }
    if (marked) {
      printf("warning:\tmarked nodes found\n");
    }
    if (locked) {
      printf("warning:\tlocked nodes found\n");
    }
    return !marked && !locked;
  }

  void display() {		// for validation, not thread-safe
    Node* node = LSentinel.next;//Note: does not work for lock-free, must use aNext. 
    char buf[128];
    while (node != &RSentinel) {
      printf("%s\n", node->toString(buf));
      node = node->next;
    }
  }
};

#endif /* _LIST_H */
