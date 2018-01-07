/**
 * lock-free list
 * teleportation
 * hazard pointer memory management
 */
#ifndef _LISTFXZ_H
#define _LISTFXZ_H
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "plat.h"
#include "List.h"
#include "ThreadLocal.h"
#include "AtomicMarkedReference.h"
#include "HazardManager.h"	// after Node declaration

class ListFXZ : public List {
public:
  static const int DEFAULT_TELEPORT_DISTANCE = 256;

public:
  HazardManager hazardManager;

  Node LSentinel;
  Node RSentinel;
    
  ListFXZ();
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
  void find(T v, Node** predPtr, Node** currPtr, ThreadLocal* threadLocal);
  Node* physicalDelete(Node* prev, Node* curr, ThreadLocal* threadLocal);
};

#endif /* _LISTFXZ_H */
