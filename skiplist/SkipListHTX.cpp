/**
 * hand-over-hand skip list
 * no memory management
 */
#include "SkipListHTZ.h"

unsigned int SkipList::randomSeed = rand();

SkipList::Node* SkipListHTZ::findNode(State* state,
		T v,
		ThreadLocal* threadLocal) {
	if (!teleport(state, v, threadLocal)) {
		fallback(state, v, threadLocal);
	}
	SkipList::Node* node = state->preds[0];
	if (node->value != v) {
		return NULL;
	} else {
		return node;
	}
};

SkipListHTZ::SkipListHTZ():
		  LSentinel(LONG_MIN, MAX_HEIGHT),
		  RSentinel(LONG_MAX, MAX_HEIGHT) {
	for (int i = 0; i < MAX_HEIGHT; i++) {
		LSentinel.next[i] = (Node*) &RSentinel;
	};
	currTopLayer = 0;
};

bool SkipListHTZ::add(T v, ThreadLocal* threadLocal) {
	bool result;
	State state;
	SkipList::Node* node = findNode(&state, v, threadLocal);
	if (node != NULL) {
		result = false;
	} else {
		node = manager.alloc(v, randomLevel(), threadLocal);
		for (int layer = 0; layer < node->height; layer++) {
			node->next[layer] = state.preds[layer]->next[layer];
			state.preds[layer]->next[layer] = node;
		}
		result = true;
	}
	unlock(state.preds);
	return result;
};

bool SkipListHTZ::remove(T v, ThreadLocal* threadLocal) {
	bool result;
	State state;
	SkipList::Node* victim = findNode(&state, v, threadLocal);
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
    			state.preds[layer]->next[layer] = victim->next[layer];
    		}
    		victim->unlock();
    		manager.retire(victim, threadLocal);
    		result = true;
	}
	unlock(state.preds);
	return result;
};

bool  SkipListHTZ::contains(T v, ThreadLocal* threadLocal) {
	bool result;
	State state;
	SkipList::Node* target = findNode(&state, v, threadLocal);
	result = (target != NULL);
	unlock(state.preds);
	return result;
};

void SkipListHTZ::unlock(Node* nodes[], int start, int stop) {
	Node* lastUnlocked = NULL;
	for (int layer = start; layer < stop; layer++) { // back out
		if (nodes[layer] != lastUnlocked) {
#if defined(DEBUG)
			if (!nodes[layer]->isLocked()) {
				printf("SkipListHTZ::unlock(%d, %d) layer %d node %s already unlocked\n",
						start,
						stop,
						layer,
						nodes[layer]->toString(buf));
				exit(1);
			}
#endif
			nodes[layer]->unlock();
			lastUnlocked = nodes[layer];
		}
	}
#if defined(DEBUG)
	if (LSentinel.isLocked()) {
		printf("error:\tunlock:\tLSentinel still locked\n");
		exit(0);
	}
	if (RSentinel.isLocked()) {
		printf("error:\tunlock:\tRSentinel still locked\n");
		exit(0);
	}
#endif
}

// state invariants:
// MAX_HEIGHT-1 >= state->level >=0
// for i > state->level
//   state->preds[i]->value < v
//   state->preds[i]->next[i]->value >= v
//   state->preds[i] locked
// state->preds[state->level]->value < v
//
// teleport postcondition:
// state->preds[state->layer] locked
// if result == true
//   layer == 0
//   state->preds[0]->next[0]->value >= v
//
bool SkipListHTZ::teleport(State* state, T v, ThreadLocal* threadLocal) {
	// first time?
	threadLocal->teleportLimit = DEFAULT_TELEPORT_DISTANCE;
	int distance = 0;
	Node* pred = state->preds[MAX_HEIGHT - 1] = &LSentinel;
#if defined(DEBUG)
	if (pred->isLocked()) {
		printf("error:\tteleport about to lock %s\n",
				pred->toString(buf)
		);
		exit(0);
	}
#endif
	pred->lock();
	bool unlockPred = true;
	Node* curr;
	int layer = MAX_HEIGHT - 1;
	// transaction retry loop
	// keep going until either
	//    we finish, or
	//    the transaction range vanishes
#if defined(DEBUG)
	Node* unlockedPred = NULL;
#endif
	while (true) {
		threadLocal->teleports++;
		if (Set::xbegin(threadLocal) == _XBEGIN_STARTED) {
			// within transaction loop
			while (true) {
				if (unlockPred) {
					pred->unlock();
					unlockPred = false;
				}
				curr = pred->next[layer];
				// layer traversal loop
				while (curr->value < v) {
					pred = curr;
					curr = curr->next[layer];
					if (distance++ > threadLocal->teleportLimit) {
						break;
					}
				}
				// pred->value < v and curr->value > v
				state->preds[layer] = pred;
				// start the next layer?
				if (curr->value >= v) {
					if (layer == MAX_HEIGHT-1 || state->preds[layer+1] != pred) {
						pred->lock();
					}
					layer--;
					if (layer >= 0) {
						state->preds[layer] = pred;
					}
				}
				// try to commit?
				if (distance > threadLocal->teleportLimit || layer < 0) {
					if (layer == MAX_HEIGHT-1 || state->preds[layer+1] != pred) {
						pred->lock();
					}
					_xend();
					state->layer = layer;
#if defined(DEBUG)
					printf("teleport commits layer = %d\n", layer);
					state->validate(v);
#endif
					threadLocal->tCommitted++;
					threadLocal->teleportDistance += distance;
					threadLocal->teleportLimit++;
					// if all layers traversed, return
					if (layer < 0) {
						return true;
					} else {
						distance = 0;
						break;
					}
				}
			}
		} else if (threadLocal->teleportLimit > 2) {
			// transaction aborted, halve teleportation range
			threadLocal->teleportLimit /= 2;
			state->layer = layer;
#if defined(DEBUG)
			printf("teleport aborts layer = %d\n", layer);
			state->validate(v);
#endif
		} else {
			// teleportation range hit bottom, save state, ask for fallback
			threadLocal->teleportLimit = DEFAULT_TELEPORT_DISTANCE;
			state->layer = layer;
#if defined(DEBUG)
			printf("teleport returns false layer %d\n",layer);
			state->validate(v);
#endif
			return false;
		}
	}
};

void SkipListHTZ::fallback(State* state, T v, ThreadLocal* threadLocal) {
	threadLocal->fallback++;
	int layer = state->layer;
	Node* pred = state->preds[layer]; // already locked
	Node* curr;
	while (layer >= 0) {
		// lock layer's next node
		curr = pred->next[layer];
		curr->lock();
		// At each loop start and finish, pred and curr both locked
		while (curr->value < v) {
			// unlock pred unless it is locked at higher level
			if (layer == MAX_HEIGHT-1 || state->preds[layer+1] != pred) {
				pred->unlock();
			}
			pred = curr;
			curr = curr->next[layer];
#if defined(DEBUG)
			if (curr->isLocked()) {
				printf("error:\tfallback2 %s should not be locked\n", curr->toString(buf));
				for (int i=MAX_HEIGHT-1; i >= layer; i--) {
					printf("state->preds[%d] is %s\n",
							layer,
							state->preds[layer]->toString(buf));
				}
				exit(0);
			}
#endif
			curr->lock();
		}
		state->preds[layer] = pred;
		layer--;
		curr->unlock();
	}
#if defined(DEBUG)
	printf("fallback returns\n");
	state->validate(v);
#endif
}
