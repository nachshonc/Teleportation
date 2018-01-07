/**
 * lock-free list
 * hazard pointers
 */
#include "ListFZ.h"

ListFZ::ListFZ():
  RSentinel(LONG_MAX),
  LSentinel(LONG_MIN, &RSentinel),
  hazardManager(2)
{
};

bool ListFZ::add(T v, ThreadLocal* threadLocal) {
  Node* curr;
  Node* pred;
  Node* node = NULL;
  while (true) {
    // find predecessor and current entries
    find(v, &pred, &curr, threadLocal);
    // is the key present?
    if (curr->value == v) {
      return false;
    } else {
      // splice in new node
      if (node == NULL) {
	node = hazardManager.alloc(v, threadLocal);
      }
      node->aNext.set(curr, false);
      if (pred->aNext.compareAndSet(curr, node, false, false)) {
	return true;
      }
    }
    Pause();
  }
};

bool ListFZ::remove(T v, ThreadLocal* threadLocal) {
  Node* curr;
  Node* pred;
  while (true) {
    // find predecessor and current entries
    find(v, &pred, &curr, threadLocal);
    // pred, curr protected
    if (curr->value != v) {
      return false;
    } else {
      // succ is not protected
      Node* succ = (Node*) curr->aNext.getReference();
      // logical deletion
      if (! curr->aNext.compareAndSet(succ, succ,
				     false, true
				     )) {
	Pause();
	continue;
      }
      // physical deletion
      if (pred->aNext.compareAndSet(curr, succ, false, false)) {
	hazardManager.retire(curr, threadLocal);
      }
      return true;
    }
  }
};

bool ListFZ::contains(T v, ThreadLocal* threadLocal) {
  Node* curr;
  Node* pred;
  find(v, &pred, &curr, threadLocal);
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

void ListFZ::find(T v, Node** predPtr, Node** currPtr, ThreadLocal* threadLocal) {
  Node* pred = NULL;
  Node* curr = NULL;
  Node* succ = NULL;
 retry: while (true) {
    // pred is protected
    pred = &LSentinel;
    // curr is protected
    curr = (Node*) hazardManager.read(&pred->aNext, threadLocal);
    while (curr->value < v) {
      while (curr->isMarked()) {
	// reread, don't overwrite pred's hazard pointer
	succ = hazardManager.reread(&curr->aNext, threadLocal);
	// pred, succ protected, curr unprotectd
	if (pred->aNext.compareAndSet(curr, succ, false, false)) {
	  // curr physical deletion worked, retire it
	  hazardManager.retire(curr, threadLocal);
	  curr = succ;
	} else {
	  // curr physical deletion failed, restart
	  goto retry;
	}
      }
      // pred, curr protected
      pred = curr;
      curr = (Node*) hazardManager.read(&curr->aNext, threadLocal);
    }
    *predPtr = pred;
    *currPtr = curr;
    return;
  }
}
