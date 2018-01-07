/**
 * lock-free list
 * hazard pointers
 */
#include "ListFXZ.h"

ListFXZ::ListFXZ():
  RSentinel(LONG_MAX),
  LSentinel(LONG_MIN, &RSentinel),
  hazardManager(2)
{
};

bool ListFXZ::add(T v, ThreadLocal* threadLocal) {
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

bool ListFXZ::remove(T v, ThreadLocal* threadLocal) {
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

bool ListFXZ::contains(T v, ThreadLocal* threadLocal) {
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

void ListFXZ::find(T v, Node** predPtr, Node** currPtr, ThreadLocal* threadLocal) {
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
// precondition: start is hazard-protected
// postcondition: result is hazard-protected
ListFXZ::Node* ListFXZ::teleport(Node* start, T v, ThreadLocal* threadLocal) {
  Node* pred = NULL;;
  Node* curr = NULL;
  Node* succ = NULL;
  bool snip;
  if (threadLocal->teleportLimit < 0) {
    threadLocal->teleportLimit = DEFAULT_TELEPORT_DISTANCE;
  }
  threadLocal->teleports++;
 retry:  while (true) {
    int distance = 0;
    if (Set::xbegin(threadLocal) == _XBEGIN_STARTED) {
      pred = start;
      curr = (Node*) pred->aNext.getReference();
      while (curr->value < v) {
	pred = curr;
	curr = (Node*) curr->aNext.getReference();
	if (distance++ > threadLocal->teleportLimit) {
	  break;
	}
      }
      hazardManager.record(pred, threadLocal);
      _xend();
      threadLocal->tCommitted++;
      threadLocal->teleportDistance += distance;
      threadLocal->teleportLimit++;
      return pred;
    } else if (threadLocal->teleportLimit > 2) { // teleport less
      threadLocal->teleportLimit /= 2;
    } else {			// non-transactional fallback
      // invariant: pred->value < v
      threadLocal->fallback++;
      pred = start;
      curr = hazardManager.read(&pred->aNext, threadLocal);
      if (curr->isMarked()) {
	curr = physicalDelete(pred, curr, threadLocal);
      }
      threadLocal->teleportLimit = 2;
      return curr;
    }
  }
};

ListFXZ::Node* ListFXZ::physicalDelete(Node* pred, Node* curr, ThreadLocal* threadLocal) {
  while (curr->isMarked()) {
    // reread, don't overwrite pred's hazard pointer
    List::Node* succ = hazardManager.reread(&curr->aNext, threadLocal);
    // pred, succ protected, curr unprotectd
    if (pred->aNext.compareAndSet(curr, succ, false, false)) {
      // curr physical deletion worked, retire
      hazardManager.retire(curr, threadLocal);
      curr = succ;
    } else {
      curr = hazardManager.read(&pred->aNext, threadLocal);
    }
  }
  return curr;
}
