/**
 * lazy list
 * teleportation
 * hazard pointers
 */
#ifndef _LISTLTZ_HPP
#define _LISTLTZ_HPP
#define null NULL
#include <limits.h>
#include "plat.h"
#include "SpinLock.h"
#include "List.h"
#include "HazardManager.h"
#include "ThreadLocal.h"


class ListLTZ : public List {
public:
//  static const int DEFAULT_TELEPORT_DISTANCE = 512;


public:
  HazardManager manager;

  ListLTZ();
  bool add(T o, ThreadLocal* threadLocal);
  bool remove(T o, ThreadLocal* threadLocal);
  bool contains(T o, ThreadLocal* threadLocal);
  inline void membar() {
    __asm__ __volatile__ ("lock;addl $0,(%%rsp);" ::: "cc") ;
  };

private:
  Node LSentinel;
  Node RSentinel;
#if defined(DEBUG)
  char buf[128];
#endif

  // General helper functions:
  bool validate(Node* pred, Node* curr);
  Node* teleport(Node* start, T v, ThreadLocal* threadLocal);

};


#endif /* _LISTLTZ_HPP */
