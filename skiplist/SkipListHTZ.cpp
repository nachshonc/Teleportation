/**
 * hand-over-hand skip list
 * no memory management
 */
#include "SkipListHTZ.h"

unsigned int SkipList::randomSeed = rand();
SpinLock SkipListHTZ::sgl;

SkipList::Node* SkipListHTZ::findNode(SkipList::Node** preds,
		T v,
		ThreadLocal* threadLocal) {
	preds[MAX_HEIGHT - 1] = &LSentinel;
	preds[MAX_HEIGHT - 1]->lock();
	for (int layer = MAX_HEIGHT - 1; layer >= 0; layer--) {
		teleport(layer, preds, v, threadLocal);
	}
#if defined(DEBUG)
	for (int layer = MAX_HEIGHT - 1; layer >= 0; layer--) {
		if (! preds[layer]->isLocked()) {
			printf("error:\tpreds[%d] = %s should be locked\n",
					layer, preds[layer]->toString(buf));
		}
	}
#endif
	SkipList::Node* node = preds[0]->next[0];
	if (node->value == v) {
		return node;
	} else {
		return NULL;
	}
};

SkipListHTZ::SkipListHTZ():
		  LSentinel(LONG_MIN, MAX_HEIGHT),
		  RSentinel(LONG_MAX, MAX_HEIGHT) {
	for (int i = 0; i < MAX_HEIGHT; i++) {
		LSentinel.next[i] = (Node*) &RSentinel;
	};
};

bool SkipListHTZ::add(T v, ThreadLocal* threadLocal) {
	bool result;
	Node* preds[MAX_HEIGHT];
	SkipList::Node* node = findNode(preds, v, threadLocal);
	if (node != NULL) {
		result = false;
	} else {
		node = manager.alloc(v, randomLevel(), threadLocal);
		for (int layer = 0; layer < node->height; layer++) {
			node->next[layer] = preds[layer]->next[layer];
			preds[layer]->next[layer] = node;
		}
		result = true;
	}
	unlock(preds);
	return result;
};

bool SkipListHTZ::remove(T v, ThreadLocal* threadLocal) {
	bool result;
	Node* preds[MAX_HEIGHT];
	SkipList::Node* victim = findNode(preds, v, threadLocal);
	if (victim == NULL) {
		result = false;
	} else {
#if defined(DEBUG)
    		if (victim->isLocked()) {
    			printf("error: remove: victim %s already locked\n",
    					victim->toString(buf));
    			exit(0);
    		}
#endif
    		victim->lock();
    		for (int layer = 0; layer < victim->height; layer++) {
    			preds[layer]->next[layer] = victim->next[layer];
    		}
    		victim->unlock();
    		manager.retire(victim, threadLocal);
    		result = true;
	}
	unlock(preds);
	return result;
};

bool SkipListHTZ::contains(T v, ThreadLocal* threadLocal) {
	bool result;
	Node* preds[MAX_HEIGHT];
	SkipList::Node* target = findNode(preds, v, threadLocal);
	result = (target != NULL);
	unlock(preds);
	return result;
};

void SkipListHTZ::unlock(SkipList::Node* nodes[], int start, int stop) {
	Node* lastUnlocked = NULL;
	bool ok = true;
	for (int layer = start; layer < stop; layer++) { // back out
		if (nodes[layer] != lastUnlocked) {
#if defined(DEBUG)
			if (!nodes[layer]->isLocked()) {
				printf("SkipListHTZ::unlock(%d, %d) layer %d node %s already unlocked\n",
						start,
						stop,
						layer,
						nodes[layer]->toString(buf));
				ok = false;
			}
#endif
			nodes[layer]->unlock();
			lastUnlocked = nodes[layer];
		}
	}
	if (!ok) exit(0);
}

// precondition
// preds[level] is locked
//
void SkipListHTZ::teleport(int layer, Node** preds, T v, ThreadLocal* threadLocal) {
	// first time?
	threadLocal->teleportLimit = DEFAULT_TELEPORT_DISTANCE;
	int distance = 0;
	// nothing to do?
	if (preds[layer]->next[layer]->value >= v) {
		if (layer > 0) {
			preds[layer - 1] = preds[layer];
		}
		return;
	}
	SkipList::Node* start = preds[layer];
	SkipList::Node* pred = start;
	SkipList::Node* curr;
	while (true) {
		threadLocal->teleports++;
#if defined(DEBUG)
		printf("%d\tteleport transaction layer = %d, limit = %d, v = %ld\n",
				threadLocal->threadNum,
				layer, threadLocal->teleportLimit, v);
#endif
		if (Set::xbegin(threadLocal) == _XBEGIN_STARTED) {
			// within transaction loop
			while (true) {
				curr = pred->next[layer];
				while (curr->value < v) {
					pred = curr;
					curr = curr->next[layer];
					if (distance++ > threadLocal->teleportLimit) {
						break;
					}
				}
				preds[layer] = pred;
				// unlock start, unless locked at higher layer
				if (layer == MAX_HEIGHT-1 || preds[layer+1] != start) {
					start->unlock();
				}
				// lock pred, unless already locked at higher level
				if (layer == MAX_HEIGHT-1 || preds[layer+1] != pred) {
					pred->lock();
				}
				// try to commit
				_xend();
#if defined(DEBUG)
				printf("%d\tteleport commits layer = %d, distance = %d, v = %ld\n",
						threadLocal->threadNum,
						layer, distance, v);
#endif
				threadLocal->tCommitted++;
				threadLocal->teleportDistance += distance;
				threadLocal->teleportLimit++;
				// finished the layer?
				if (curr->value >= v) {
					if (layer > 0) {
						preds[layer - 1] = pred;
					}
#if defined(DEBUG)
					printf("%d\tteleport returns curr->value = %ld, v = %ld, limit = %d\n",
							threadLocal->threadNum,
							curr->value, v,
							threadLocal->teleportLimit);
#endif
					return;
				}
			}
		} else if (threadLocal->teleportLimit > 4) {
			// transaction aborted, halve teleportation range
			threadLocal->teleportLimit /= 2;
#if defined(DEBUG)
			printf("%d\tteleport aborts layer = %d, limit = %d, v = %ld\n",
					threadLocal->threadNum,
					layer, threadLocal->teleportLimit, v);
#endif
		} else {
#if defined(DEBUG)
			printf("%d\tteleport fallback layer = %d, limit = %d, v = %ld\n",
					threadLocal->threadNum,
					layer, threadLocal->teleportLimit, v);
#endif
			// teleportation range hit bottom, fall back to hand-over-hand
			pred = preds[layer];	// already locked
			curr = pred->next[layer];
			curr->lock();
			while (curr->value < v) {
				// unlock pred unless it is locked at higher level
				if (layer == MAX_HEIGHT-1 || preds[layer+1] != pred) {
					pred->unlock();
				}
				pred = curr;
				curr = curr->next[layer];
				curr->lock();
			}
			// end of layer, move down and initialize next layer
			preds[layer] = pred;
			if (layer > 0) {
				preds[layer - 1] = pred;
			}
			curr->unlock();
			return;
		}
	}
};
