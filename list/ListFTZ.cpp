/**
 * lock-free list
 * teleportation
 * hazard pointer memory management
 */
#include "ListFTZ.h"

ListFTZ::ListFTZ():
  RSentinel(LONG_MAX),
  LSentinel(LONG_MIN, &RSentinel),
  hazardManager(3) { };

bool ListFTZ::add(T v, ThreadLocal* threadLocal) {
  bool result = false;
 restart:
  Node* pred= &LSentinel;
  Node* curr = NULL;
  Node* node = NULL;
  // loop invariant: pred is protected. Note that LSentinel doesn't need protection
  while (true) {
    curr = hazardManager.read(&pred->aNext, threadLocal);
    if (curr->value == v) {
      return false;
    } else if (curr->value > v) {
      if (node == NULL) {
	node = hazardManager.alloc(v, threadLocal);
      }
      node->aNext.set(curr, false);
      if (pred->aNext.compareAndSet(curr, node, false, false)) {
	return true;
      }
    } else {
      pred = teleport(pred, v, threadLocal);
    }
  }
};

bool ListFTZ::remove(T v, ThreadLocal* threadLocal) {
  bool result = false;
 restart:
  Node* pred= &LSentinel;
  Node* curr = NULL;
  while (true) {
    // pred is protected
    curr = hazardManager.read(&pred->aNext, threadLocal);
    // curr is protected
    if (curr->value > v) {
      return false;
    } else if (curr->value == v) {
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
	//	hazardManager.retire(curr, threadLocal);
      }
      return true;
    } else {
      pred = teleport(pred, v, threadLocal);
    }
  }
};

bool ListFTZ::contains(T v, ThreadLocal* threadLocal) {
  
  Node* pred= &LSentinel;
  while (true) {
    Node* curr = hazardManager.read(&pred->aNext, threadLocal);
    if (curr->value == v) {
      return  curr->value == v && !curr->isMarked();
    } else if (curr->value > v) {
      return false;
    } else {
      pred = teleport(pred, v, threadLocal);
    }
  }
};

// precondition: start is hazard-protected
// postcondition: result is hazard-protected
ListFTZ::Node* ListFTZ::teleport(Node* start, T v, ThreadLocal* threadLocal) {
	Node* pred = NULL;;
	Node* curr = NULL;
	Node* succ = NULL;
	bool snip;
	int retries = RETRIES;
retry:	while(--retries){
		threadLocal->teleports++;
		int distance = 0;
		if (Set::xbegin(threadLocal) == _XBEGIN_STARTED) {
			pred = start;
			curr = (Node*) pred->aNext.getReference();
			while (curr->value < v) {
				if (curr->isMarked()) {
					pred->aNext = curr->aNext;
					//	  hazardManager.retire(curr, threadLocal);
					curr = (Node*) pred->aNext.getReference();
				} else {
					pred = curr;
					curr = (Node*) curr->aNext.getReference();
				}
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
		} else {
			threadLocal->teleportLimit=(threadLocal->teleportLimit>2*MIN_TEL_LIMIT)?threadLocal->teleportLimit/2:MIN_TEL_LIMIT;
		}
	}
	{// non-transactional fallback
		// invariant: pred->value < v
		threadLocal->fallback++;
		pred = start;
		curr = hazardManager.read(&pred->aNext, threadLocal);
		while (curr->isMarked()) {
			// reread, don't overwrite pred's hazard pointer
			succ = hazardManager.reread(&curr->aNext, threadLocal);
			// pred, succ protected, curr unprotectd
			if (pred->aNext.compareAndSet(curr, succ, false, false)) {
				// curr physical deletion worked, retire
				//	  hazardManager.retire(curr, threadLocal);
				curr = succ;
			} else {
				// curr physical deletion failed, restart
				goto retry;
			}
		}
		return curr;
	}
};
