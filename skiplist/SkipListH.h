/**
 * Lazy Skiplist
 * author: Maurice Herlihy
 */
#ifndef _SKIPLISTH_H
#define _SKIPLISTH_H
#include "SkipList.h"
#include "SimpleManager.h"

class SkipListH: public SkipList {
public:
  SkipListH();
  bool add(T o, ThreadLocal* threadLocal);
  bool remove(T o, ThreadLocal* threadLocal);
  bool contains(T o, ThreadLocal* threadLocal);
  // storage
  SimpleManager manager;

  int size();
  bool sanity();
  void display();

private:
  Node LSentinel;
  Node RSentinel;
#if defined(DEBUG)
  char buf[128];		// debugging messages
#endif
  volatile int currTopLayer;
  // helper functions:
  void unlock(Node* nodes[], int start, int stop);
  inline void unlock(Node* nodes[]) {unlock(nodes, 0, MAX_HEIGHT);};
  void find(T v,
	    Node* preds[],
	    ThreadLocal* threadLocal);
  unsigned int xbegin(ThreadLocal* threadLocal);
};

#endif /* _SKIPLISTH_H */
