/**
 * generic skip list header
 */
#ifndef _SKIPLIST_H
#define _SKIPLIST_H
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
#include <assert.h>
#define null NULL
#ifdef DEBUG
#define DASSERT(c) assert(c)
#else
#define DASSERT(c)
#endif

class SkipList: public Set {

#if defined(DEBUG)
protected:
  char buf[512];
#endif
public:
  const static int MAX_HEIGHT = 8;
  static unsigned int randomSeed;
  SkipList():
    LSentinel(LONG_MIN, MAX_HEIGHT),
    RSentinel(LONG_MAX, MAX_HEIGHT) {
    for (int i = 0; i < MAX_HEIGHT; i++) {
      LSentinel.next[i] = (Node*) &RSentinel;
    };
  }

  static int randomLevel() {
    unsigned int x = randomSeed;
    x ^= x << 13;
    x ^= x >> 17;
    randomSeed = x ^= x << 5;
    if ((x & 0x8001) != 0) // test highest and lowest bits
      return 1;
    int level = 2;
    while (((x >>= 1) & 1) != 0) ++level;
    return (level < MAX_HEIGHT) ? level : MAX_HEIGHT;
  }

  class Node {
  public:
    SpinLock spinLock;
    long value;
    int height;
    Node* next[SkipList::MAX_HEIGHT];
    volatile bool marked;
    volatile bool fullyLinked;
    Node* free;
    enum NODE_STATE{STATE_RETIRE, STATE_FREE, STATE_ALLOC};
    //int node_state;
    void change_state(int old, int news){
    		//assert(__sync_bool_compare_and_swap(&node_state, old, news));
    }

    Node(long val, int _height): marked(false),
				 fullyLinked(false),
				 height(_height),
				 value(val) {
    };
    void set(long val, int _height){
    		marked=false;
    		fullyLinked=false;
    		height=_height;
    		value=val;
    }
    Node(long val): marked(false),
		    fullyLinked(false),
		    height(randomLevel()),
		    value(val) {
    };
    Node(): marked(false),
	    fullyLinked(false),
	    height(randomLevel()) {
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
    char* toString(char* buf) {
      if (value == LONG_MIN) {
	sprintf(buf, "LSentinel,%s",
		isLocked() ? "locked" : "unlocked");
      } else if (value == LONG_MAX) {
	sprintf(buf, "RSentinel,%s",
		isLocked() ? "locked" : "unlocked");
      } else {
	sprintf(buf, "Node@%p->%ld,%s", this,
		value,
		isLocked() ? "locked" : "unlocked");
      }
      return buf;
    }
  };

public:
  Node LSentinel;
  Node RSentinel;
  inline void membar() {
    __asm__ __volatile__ ("lock;addl $0,(%%rsp);" ::: "cc") ;
  };
  int size() {		// for validation, not thread-safe
    int result = 0;
    Node* node = LSentinel.next[0];
    while (node != &RSentinel) {
      if (!node->marked) {
	result++;
      }
      node = node->next[0];
    }
    return result;
  }

  bool sanity() {		// for validation, not thread-safe
    bool marked = false;
    bool locked = false;
    Node* node = LSentinel.next[0];
    while (node != &RSentinel) {
      if (node->marked) {
	marked = true;
      }
      if (node->isLocked()) {
	locked = true;
      }
      node = node->next[0];
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
    Node* node = LSentinel.next[0];
    char buf[128];
    while (node != &RSentinel) {
      printf("%s\n", node->toString(buf));
      node = node->next[0];
    }
    printf("layer\tlength\n");
    for (int layer = 0; layer < MAX_HEIGHT; layer++) {
      int length = 0;
      node = &LSentinel;
      while (node != &RSentinel) {
	length++;
	node = node->next[layer];
      }
      printf("%d\t%d\n", layer, length);
    }
  }
};

#endif /* _SKIPLIST_H */
