/**
 * lazy list
 * teleportation
 * hazard pointers
 */
#include "ListLTZ.h"

ListLTZ::ListLTZ():
LSentinel(LONG_MIN),
RSentinel(LONG_MAX),
manager(2) {
	LSentinel.next = (Node*) &RSentinel;
};

bool ListLTZ::validate(Node* pred, Node* curr) {
	return (!pred->marked) && (!curr->marked) && (pred->next == curr);
};

bool ListLTZ::add(T v, ThreadLocal* threadLocal) {
	bool result = false;
	restart:
	Node* pred= &LSentinel;
	Node* curr = NULL;
	while (true) {
		// pred is protected
		curr = manager.read(&pred->next, threadLocal);
		// curr is protected
		if (curr->value >= v) {
			pred->lock();
			curr->lock();
			if (!validate(pred, curr)) {
				pred->unlock();
				curr->unlock();
				Pause();
				goto restart;
			} else if (curr->value == v) {
				result = false;
				break;
			} else {
				Node* node = manager.alloc(v, threadLocal);
				node->next = curr;
				pred->next = node;
				result = true;
				break;
			}
		} else {
			pred = teleport(pred, v, threadLocal);
			if (pred->marked) {	// hazard poiner hazard
				Pause();
				goto restart;
			}
		}
	}
	pred->unlock();
	curr->unlock();
	return result;
};


bool ListLTZ::remove(T v, ThreadLocal* threadLocal) {
	bool result = false;
	restart:
	Node* pred= &LSentinel;
	Node* curr = NULL;
	while (true) {
		curr = manager.read(&pred->next, threadLocal);
		if (curr->value >= v) {
			pred->lock();
			curr->lock();
			if (!validate(pred, curr)) {
				pred->unlock();
				curr->unlock();
				Pause();
				goto restart;
			} else if (curr->value > v) {
				result = false;
				break;
			} else {
				curr->marked = true;
				pred->next = curr->next;
				manager.retire(curr, threadLocal);
				result = true;
				break;
			}
		} else {
			pred = teleport(pred, v, threadLocal);
			if (pred->marked) {	// hazard pointer hazard
				Pause();
				goto restart;
			}
		}
	}
	pred->unlock();
	curr->unlock();
	return result;
};

bool ListLTZ::contains(T v, ThreadLocal* threadLocal) {
	Node* pred= &LSentinel;
	restart:
	while (true) {
		Node* curr = manager.read(&pred->next, threadLocal);
		if (curr->value == v) {
			return  curr->value == v && !curr->marked;
		} else if (curr->value > v) {
			return false;
		} else {
			pred = teleport(pred, v, threadLocal);
			if (pred->marked) {	// hazard pointer hazard
				Pause();
				goto restart;
			}
		}
	}
};

// precondition: start is hazard-protected
// postcondition: result is hazard-protected
ListLTZ::Node* ListLTZ::teleport(Node* start, T v, ThreadLocal* threadLocal) {
	Node* pred = start;
	Node* curr;
	/*if (threadLocal->teleportLimit < 0) {
    threadLocal->teleportLimit = DEFAULT_TELEPORT_DISTANCE;
  }*/
	int retries = RETRIES;
	while (--retries) {
		threadLocal->teleports++;
		int distance = 0;
		if (Set::xbegin(threadLocal) == _XBEGIN_STARTED) {
			curr = start->next;
			while (curr->value < v) {
				pred = curr;
				curr = curr->next;
				if (distance++ > threadLocal->teleportLimit) {
					break;
				}
			}
			manager.record(pred, threadLocal);
			_xend();
			threadLocal->tCommitted++;
			threadLocal->teleportDistance += distance;
			threadLocal->teleportLimit++;
#if defined(DEBUG)
			if (start == pred) {
				printf("error:\tteleport returns start\n");
			}
#endif
			return pred;
		} else {
			threadLocal->teleportLimit=(threadLocal->teleportLimit>2*MIN_TEL_LIMIT)?threadLocal->teleportLimit/2:MIN_TEL_LIMIT;
		}
	}
	{// non-transactional fallback
		threadLocal->fallback++;
		curr = manager.read(&pred->next, threadLocal);
		threadLocal->teleportDistance++;
		//threadLocal->teleportLimit = 2;
		return curr;
	}
};

