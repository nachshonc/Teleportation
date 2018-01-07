/**
 * Lazy list
 * epoch based
 */
#include "ListLE.h"

ListLE::ListLE():
LSentinel(LONG_MIN),
RSentinel(LONG_MAX) {
	LSentinel.next = (Node*) &RSentinel;
};

bool ListLE::validate(Node* pred, Node* curr) {
	return (!pred->marked) && (!curr->marked) && (pred->next == curr);
};

bool ListLE::add(T v, ThreadLocal* threadLocal) {
	Node* pred;
	Node* curr;
	bool result = false;
	int retryCount = 0;
	epochManager.enter(threadLocal);
	while (true) {
		pred = &LSentinel;
		curr = pred->next;
		while (curr->value < v) {
#if defined(DEBUG)
			if (curr->value == 0xdeadbeef) {
				printf("error:\tdeadbeef encounted\n");
				exit(0);
			}
			if (curr->value >= curr->next->value) {
				printf("error:\tadd monotonicity violation\n");
				exit(0);
			}
#endif
			pred = curr;
			curr = curr->next;
		}
		pred->lock();
		curr->lock();
		if (!validate(pred, curr)) {
			pred->unlock();
			curr->unlock();
			Pause();
			continue;
		} else if (curr->value == v) {
			result = false;
			break;
		} else {
			Node* node = epochManager.alloc(v, threadLocal);
			node->next = curr;
			pred->next = node;
			result = true;
			break;
		}
#if defined(debug)
    		if (++retryCount % 10000 == 0) {
    			printf("add retry(s) %d\n", retryCount);
    			exit(0);
    		}
#endif
	}
	pred->unlock();
	curr->unlock();
	epochManager.exit(threadLocal);
	return result;
};


bool ListLE::remove(T v, ThreadLocal* threadLocal) {
	Node* pred;
	Node* curr;
	bool result = false;
	int retryCount = 0;

	epochManager.enter(threadLocal);
	while (true) {
		pred = &LSentinel;
		curr = pred->next;
		while (curr->value < v) {
#if defined(DEBUG)
			if (curr->value == 0xdeadbeef) {
				printf("error:\tdeadbeef encounted\n");
				exit(0);
			}
			if (curr->value >= curr->next->value) {
				printf("error:\tremove monotonicity violation\n");
				exit(0);
			}
#endif
			pred = curr;
			curr = curr->next;
		}
		pred->lock();
		curr->lock();
		if (!validate(pred, curr)) {
			pred->unlock();
			curr->unlock();
			Pause();
			continue;
		} else if (curr->value == v) {
			curr->marked = true;
			pred->next = curr->next;
			curr->unlock();
			epochManager.retire(curr, threadLocal);
			result = true;
			break;
		} else {
			curr->unlock();
			result = false;
			break;
		}
#if defined(DEBUG)
    		if (++retryCount % 10000 == 0) {
    			printf("warning:\tremove retry %d\n", retryCount);
    			exit(0);
    		}
#endif
	}
	pred->unlock();
	epochManager.exit(threadLocal);
	return result;
};

bool ListLE::contains(T v, ThreadLocal* threadLocal) {
	epochManager.enter(threadLocal);
	Node* curr = LSentinel.next;
	while (curr->value < v) {
#if defined(DEBUG)
		if (curr->value == 0xdeadbeef) {
			printf("error:\tdeadbeef encounted\n");
			exit(0);
		}
		if (curr->value >= curr->next->value) {
			printf("error:\tcontains monotonicity violation\n");
			exit(0);
		}
#endif
		curr = curr->next;
	}
	epochManager.exit(threadLocal);
	return curr->value == v && !curr->marked;
};
