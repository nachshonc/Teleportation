/**
 * Lazy list with hazard pointers
 */
#ifndef _LISTLZ_HPP
#define _LISTLZ_HPP
#define null NULL
#include <limits.h>
#include "plat.h"
#include "SpinLock.h"
#include "List.h"
#include "HazardManager.h"
#include "ThreadLocal.h"

class ListLZ : public List {
public:

  HazardManager hazardManager;
  ListLZ();
  bool add(T o, ThreadLocal* threadLocal);
  bool remove(T o, ThreadLocal* threadLocal);
  bool contains(T o, ThreadLocal* threadLocal);

  List::Node *findNode(T v, ThreadLocal* threadLocal, List::Node **prev = NULL);

//  inline void membar() {
//    __asm__ __volatile__ ("lock;addl $0,(%%rsp);" ::: "cc") ;
//  };
  int size() {		// for validation, not thread-safe
    int result = 0;
    Node* node = LSentinel.next;
    while (node != &RSentinel) {
      if (!node->marked) {
	result++;
      }
      node = node->next;
    }
    return result;
  }

  bool sanity() {		// for validation, not thread-safe
    bool marked = false;
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
    Node* node = LSentinel.next;
    char buf[128];
    while (node != &RSentinel) {
      printf("%s\n", node->toString(buf));
      node = node->next;
    }
  }


private:
  Node LSentinel;
  Node RSentinel;
#if defined(DEBUG)
  char buf[128];
#endif

  // General helper functions:
  int randomLevel();
  bool validate(Node* pred, Node* curr);
    
};


#endif /* _LISTLZ_HPP */
