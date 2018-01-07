/**
 * hand-over-hand list
 * teleportation
 * hazard pointer memory management
 */
#include "ListHTZ.h"

ListHTZ::ListHTZ():
LSentinel(LONG_MIN),
RSentinel(LONG_MAX) {
	LSentinel.next = (Node*) &RSentinel;
};

bool ListHTZ::add(T v, ThreadLocal* threadLocal) {
	Node local;local.next=&LSentinel; local.spinLock.val=local.spinLock.LOCKED;
	Node* pred = &local;
	bool result = false;
	//pred = &LSentinel;  pred->lock();
	while (true) {
		if (pred->next->value == v) {
			result = false;
			break;
		} else if (pred->next->value > v) {
			Node* node = manager.alloc(v, threadLocal);
			node->next = pred->next;
			pred->next = node;
			result = true;
			break;
		} else {
			pred = teleport(pred, v, threadLocal);
		}
	}
	pred->unlock();
	return result;
};


bool ListHTZ::remove(T v, ThreadLocal* threadLocal) {
	Node local;local.next=&LSentinel; local.spinLock.val=local.spinLock.LOCKED;
	Node* pred = &local;
	bool result = false;
	int retryCount = 0;
	//pred = &LSentinel;  pred->lock();
	while (true) {
		if (pred->next->value > v) {
			result = false;
			break;
		} else if (pred->next->value == v) {
			Node* nodeToDelete = pred->next;
			nodeToDelete->lock();
			pred->next = nodeToDelete->next;
			nodeToDelete->unlock();
			manager.retire(nodeToDelete, threadLocal);
			result = true;
			break;
		} else {
			pred = teleport(pred, v, threadLocal);
		}
	}
	pred->unlock();
	return result;
};

bool ListHTZ::contains(T v, ThreadLocal* threadLocal) {
	Node local;local.next=&LSentinel; local.spinLock.val=local.spinLock.LOCKED;
	Node* pred = &local;
	bool result = false;
	//pred = &LSentinel;
	//pred->lock();
	while (true) {
		if (pred->next->value == v) {
			result = true;
			break;
		} else if (pred->next->value > v) {
			result = false;
			break;
		} else {
			pred = teleport(pred, v, threadLocal);
		}
	}
	pred->unlock();
	return result;
};


// precondition: start is locked
// postcondition: result is locked
ListHTZ::Node* ListHTZ::teleport(Node* start, T v, ThreadLocal* threadLocal) {
	Node* pred = start;
	Node* curr = start->next; //pred is locked, so this is constant.
	int distance = 0;
	int retries = RETRIES;
	while(--retries){
		threadLocal->teleports++;
		if (Set::xbegin(threadLocal) == _XBEGIN_STARTED) {
			while (curr->value < v) {
				pred = curr;
				curr = curr->next;
				if (distance++ > threadLocal->teleportLimit) {
					break;
				}
			}
			pred->lock();
			start->unlock();
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
		}
		else{
			threadLocal->teleportLimit=(threadLocal->teleportLimit>2*MIN_TEL_LIMIT)?threadLocal->teleportLimit/2:MIN_TEL_LIMIT;
		}
	}
	{			// non-transactional fallback
		threadLocal->fallback++;
		curr->lock();
		start->unlock();
		threadLocal->teleportDistance++;
		//threadLocal->teleportLimit = 2;
		return curr;
	}
}


int ListHTZ::size() {
	int result = 0;
	Node* node = LSentinel.next;
	while (node != &RSentinel) {
		result++;
		node = node->next;
	}
	return result;
}
bool ListHTZ::sanity() {
	Node* node = LSentinel.next;
	bool ok = true;
	char buf[128];
	while (node != &RSentinel) {
		if (node->isLocked()) {
			ok = false;
			printf("error:\tnode %s is locked\n", node->toString(buf));
		}
		node = node->next;
	}
	return ok;
}

void ListHTZ::display() {		// for validation, not thread-safe
	Node* node = LSentinel.next;
	char buf[128];
	while (node != &RSentinel) {
		printf("%s\n", node->toString(buf));
		node = node->next;
	}
}
