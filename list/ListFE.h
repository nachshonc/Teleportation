/**
 * lock-free list
 * epoch-based reclamation
 */
#ifndef _LISTFE_HPP
#define _LISTFE_HPP
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "plat.h"
#include "List.h"
#include "ThreadLocal.h"
#include "AtomicMarkedReference.h"
#include "EpochManager.h"

class ListFE : public List {

public:

  Node LSentinel;
  Node RSentinel;
  EpochManager epochManager;
    
  ListFE();
  bool add(T o, ThreadLocal* threadLocal);
  bool remove(T o, ThreadLocal* threadLocal);
  bool contains(T o, ThreadLocal* threadLocal);
private:
#if defined(DEBUG)
  char buf[128];
#endif
  // General helper functions:
  void find(T v, Node** predPtr, Node** currPtr, ThreadLocal* threadLocal);
};

#endif /* _LISTFE_HPP */
