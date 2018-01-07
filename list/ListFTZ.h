/**
 * lock-free list
 * teleportation
 * hazard pointer memory management
 */
#ifndef _LISTFTZ_H
#define _LISTFTZ_H
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "plat.h"
#include "List.h"
#include "ThreadLocal.h"
#include "AtomicMarkedReference.h"
#include "HazardManager.h"	// after Node declaration

class ListFTZ : public List {
public:
  HazardManager hazardManager;

  Node LSentinel;
  Node RSentinel;
    
  ListFTZ();
  bool add(T o, ThreadLocal* threadLocal);
  bool remove(T o, ThreadLocal* threadLocal);
  bool contains(T o, ThreadLocal* threadLocal);
  int size() {		// for validation, not thread-safe
    int result = 0;
    Node* node = (Node*) LSentinel.aNext.getReference();
    while (node != &RSentinel) {
      if (!node->aNext.isMarked()) {
	result++;
      }
      node = (Node*) node->aNext.getReference();
    }
    return result;
  }

private:
  Node* teleport(Node* start, T v, ThreadLocal* threadLocal);
  Node* physicalDelete(Node* prev, Node* curr, ThreadLocal* threadLocal);
};

#endif /* _LISTFTZ_H */
