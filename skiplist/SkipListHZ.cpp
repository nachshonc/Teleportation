/**
 * hand-over-hand skip list
 * no memory management
 */
#include "SkipListHZ.h"

unsigned int SkipList::randomSeed = rand();

void SkipListHZ::find(T v, Node* preds[], ThreadLocal* threadLocal) {
	Node* pred = &LSentinel;
	pred->lock();
	// At each loop start, pred locked, pred->value < v
	for (int layer = MAX_HEIGHT-1; layer >=0; layer--) {
		// lock layer's first node
		Node* curr = pred->next[layer];
		curr->lock();
		// At each loop start and finish, pred and curr both locked
		while (curr->value < v) {
			// unlock pred unless it is locked at higher level
			if (layer == MAX_HEIGHT-1 || preds[layer+1] != pred) {
				pred->unlock();
			}
			pred = curr;
			curr = curr->next[layer];
			curr->lock();
		}
		// pred->value < v, curr->value >= v, both locked
		preds[layer] = pred;
#if defined(DEBUG)
		if (! curr->isLocked()) {
			printf("error:\tSkipListHZ::find:\tlayer %d, curr = %s not locked\n",
					layer,
					curr->toString(buf));
			exit(0);
		}
		if (! pred->isLocked()) {
			printf("error:\tSkipListHZ::find:\tlayer %d, pred = %s not locked\n",
					layer,
					pred->toString(buf));
			exit(0);
		}
		if (! preds[layer]->isLocked()) {
			printf("error:\tSkipListH:Z:find:\t preds[%d] = %s not locked\n",
					layer, preds[layer]->toString(buf));
			exit(0);
		}
#endif
		curr->unlock();
	}
}

SkipListHZ::SkipListHZ():
		  LSentinel(LONG_MIN, MAX_HEIGHT),
		  RSentinel(LONG_MAX, MAX_HEIGHT) {
	for (int i = 0; i < MAX_HEIGHT; i++) {
		LSentinel.next[i] = (Node*) &RSentinel;
	};
	currTopLayer = 0;
};

bool SkipListHZ::add(T v, ThreadLocal* threadLocal) {
	bool result;
	Node* preds[MAX_HEIGHT];
	find(v, preds, threadLocal);
	if (preds[0]->next[0]->value == v) {
		result = false;
	} else {
		Node* node = manager.alloc(v, randomLevel(), threadLocal);
		for (int layer = 0; layer < node->height; layer++) {
			node->next[layer] = preds[layer]->next[layer];
			preds[layer]->next[layer] = node;
		}
		result = true;
	}
	unlock(preds);
	return result;
};

bool SkipListHZ::remove(T v, ThreadLocal* threadLocal) {
	bool result;
	Node* preds[MAX_HEIGHT];
	find(v, preds, threadLocal);
	if (preds[0]->next[0]->value > v) {
		result = false;
	} else {
		Node* nodeToDelete = preds[0]->next[0];
		nodeToDelete->lock();
		for (int layer = 0; layer < nodeToDelete->height; layer++) {
			preds[layer]->next[layer] = nodeToDelete->next[layer];
		}
		manager.retire(nodeToDelete, threadLocal);
		result = true;
	}
	unlock(preds);
	return result;
};

bool  SkipListHZ::contains(T v, ThreadLocal* threadLocal) {
	Node* pred = &LSentinel;
	pred->lock();
	for (int layer = MAX_HEIGHT-1; layer >=0; layer--) {
		Node* curr = pred->next[layer];
		while (curr->value < v) {
			curr->lock();
			pred->unlock();
			pred = curr;
			curr = pred->next[layer];
		}
		if (curr->value == v) {
			pred->unlock();
			return true;
		}
	}
	pred->unlock();
	return false;
};

int SkipListHZ::size() {		// for validation, not thread-safe
	int result = 0;
Node* node = LSentinel.next[0];
while (node != &RSentinel) {
	result++;
	node = node->next[0];
}
return result;
}

void SkipListHZ::display() {		// for validation, not thread-safe
	Node* node = LSentinel.next[0];
	char buf[128];
	while (node != &RSentinel) {
		printf("%s\n", node->toString(buf));
		node = node->next[0];
	}
}

bool SkipListHZ::sanity() {
	Node* node = LSentinel.next[0];
	bool ok = true;
	char buf[128];
	while (node != &RSentinel) {
		if (node->isLocked()) {
			ok = false;
			printf("error:\tnode %s is locked\n", node->toString(buf));
		}
		node = node->next[0];
	}
	return ok;
}

void SkipListHZ::unlock(Node* nodes[], int start, int stop) {
	Node* lastUnlocked = NULL;
	for (int layer = start; layer < stop; layer++) { // back out
		if (nodes[layer] != lastUnlocked) {
#if defined(DEBUG)
			if (!_xtest()) {
				if (!nodes[layer]->isLocked()) {
					printf("SkipListHZ::unlock(%d, %d) layer %d node %s already unlocked\n",
							start,
							stop,
							layer,
							nodes[layer]->toString(buf));
					exit(1);
				}
			}
#endif
			nodes[layer]->unlock();
			lastUnlocked = nodes[layer];
		}
	}
}
