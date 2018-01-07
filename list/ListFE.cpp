/**
 * lock-free list no memory management
 */
#include "ListFE.h"

ListFE::ListFE():
  RSentinel(LONG_MAX),
  LSentinel(LONG_MIN, &RSentinel){ };

bool ListFE::add(T v, ThreadLocal* threadLocal) {
  Node* curr;
  Node* pred;
  Node* node = NULL;
  epochManager.enter(threadLocal);
  bool result;
  while (true) {
    // find predecessor and current entries
    find(v, &pred, &curr, threadLocal);
    // is the key present?
    if (curr->value == v) {
      result = false;
      break;
    } else {
      // splice in new node
      if (node == NULL) {
	node = epochManager.alloc(v, threadLocal);
      }
      node->aNext.set(curr, false);
      if (pred->aNext.compareAndSet(curr, node, false, false)) {
	result = true;
	break;
      }
    }
    Pause();
  }
  epochManager.exit(threadLocal);
  return result;
};

bool ListFE::remove(T v, ThreadLocal* threadLocal) {
  Node* curr;
  Node* pred;
  epochManager.enter(threadLocal);
  bool result;
  while (true) {
    // find predecessor and current entries
    find(v, &pred, &curr, threadLocal);
    // is the key present?
    if (curr->value != v) {
      result = false;
      break;
    } else {
      Node* succ = (Node*)curr->aNext.getReference();
      // logical deletion
      if (! curr->aNext.compareAndSet(succ, succ,
				     false, true
				     )) {
	Pause();
	continue;
      }
      // physical deletion
      if (pred->aNext.compareAndSet(curr, succ, false, false)) {
	epochManager.retire(curr, threadLocal);
      }
      result= true;
      break;
    }
  }
  epochManager.exit(threadLocal);
  return result;
};

bool  ListFE::contains(T v, ThreadLocal* threadLocal) {
  Node* curr = (Node*) LSentinel.aNext.getReference();
  epochManager.enter(threadLocal);
  while (curr->value < v) {
    curr = (Node*) curr->aNext.getReference();
  }
  epochManager.exit(threadLocal);
  return curr->value == v && !curr->isMarked();
};


/**
 * If element is present, returns node and predecessor. If absent, returns
 * node with least larger key.
 * @param head start of list
 * @param key key to search for
 * @return If element is present, returns node and predecessor. If absent, returns
 * node with least larger key.
 */
void ListFE::find(T v, Node** predPtr, Node** currPtr, ThreadLocal* threadLocal) {
  Node* pred = NULL;
  Node* curr = NULL;
  Node* succ = NULL;
 retry: while (true) {
    pred = &LSentinel;
    curr = (Node*) pred->aNext.getReference();
    while (curr->value < v) {
      while (curr->isMarked()) {
	succ = (Node*) curr->aNext.getReference();
	if (pred->aNext.compareAndSet(curr, succ, false, false)) {
	  epochManager.retire(curr, threadLocal);
	  curr =(Node*) pred->aNext.getReference();
	} else {
	  // curr physical deletion failed, restart
	  goto retry;
	}
      }
      pred = curr;
      curr =(Node*) curr->aNext.getReference();
    }
    *predPtr = pred;
    *currPtr = curr;
    return;
  }
};
