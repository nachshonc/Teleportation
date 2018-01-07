/**
 * Lazy Skiplist
 * hazard pointers
 * teleportaton
 * author: Maurice Herlihy
 */
#include "SkipListLTZ.h"
#if defined(DEBUG)
#define LOOP_WARNING 1024 * 1024
#endif

unsigned int SkipList::randomSeed = rand();

// findNode: looks for a given key.
// If key in the list, returns the node containing
// this key, and update appropriate location in preds. 
// Otherwise returns null.
//
SkipList::Node* SkipListLTZ::findNode(State* state,
		T v,
		ThreadLocal* threadLocal) {
	state->layerFound = -1;
	state->pred = &LSentinel;
	state->layer = MAX_HEIGHT - 1;
	if (!teleport(state, v, threadLocal)) {
		fallback(state, v, threadLocal);
	}
	if (state->curr->value != v) {
		return NULL;
	} else {
		return state->curr;
	}
};

SkipListLTZ::SkipListLTZ():
		  manager(MAX_HEIGHT) {
	currTopLayer = 0;
};

bool SkipListLTZ::validate(Node* pred, Node* curr, int layer) {
	return (!pred->marked) && (!curr->marked) && (pred->next[layer]==curr);
};

bool SkipListLTZ::weakValidate(Node* pred, Node* curr, int layer) {
	return (!pred->marked) && (pred->next[layer]==curr);
};


bool SkipListLTZ::add(T v, ThreadLocal* threadLocal) {
	int nodeTopLayer = randomLevel();
	int markedCount = 0;
	int linkedCount = 0;
	int validCount = 0;
	unsigned int backoff;
	struct timespec timeout;
	State state;

	Node* newNode = manager.alloc(v, nodeTopLayer, threadLocal);
	while (true) {
		state.layerFound = -1;
		state.pred = &LSentinel;
		int highestLockedLayer = -1;
		bool valid = true;
		Node* node = findNode(&state, v, threadLocal);
		if (node != NULL) {
			if (!node->marked) {
				int spinCount = 0;
				while (! node->fullyLinked) {
#if defined(DEBUG)
					if (++linkedCount > LOOP_WARNING) {
						printf("warning: add() fully linked retry %d\n", linkedCount);
						exit(0);
					}
#endif
				} // spin
				return false;
			}
#if defined(DEBUG)
			if (++markedCount > LOOP_WARNING) {
				printf("warning: add() marked retry %d\n", markedCount);
				exit(0);
			}
#endif
			Pause();
			continue;
		}

		// 2. Validate and lock predecessors:
		//
		Node* prevPred = NULL;
		for (int layer = 0; valid && (layer < nodeTopLayer); layer++) {
			Node* pred = state.preds[layer];
			Node* curr = state.succs[layer];
			if (pred != prevPred) {
				pred->lock();
				highestLockedLayer = layer;
				prevPred = pred;
			}
			valid = valid && validate(pred, curr, layer);
		}
		if (!valid) {   // If validation failed, retry:
			// unlock predecessors
			unlockPreds(highestLockedLayer, state.preds);
			//       if (backoff > 5000) {
			// 	timeout.tv_sec = backoff / 5000;
			// 	timeout.tv_nsec = (backoff % 5000) * LOOP_WARNING0;
			// 	nanosleep(&timeout, NULL);
			//       }
			//       backoff *= 2;
#if defined(DEBUG)
			if (++validCount > LOOP_WARNING) {
				printf("warning: add() valid retry %d\n", validCount);
				exit(0);
			}
#endif
			Pause();
			continue;
		}

		// 3. Create and link node in all layers.
		// We linearize only when the node is fully in, to save
		// retries of a concurrent remove operation that sees a
		// partially inserted node. We use the fullyLinked
		// field to mark when insertion in all layers is done.
		//
		// For it to be a valid linearization point, we must
		// mark the field after we guarantee that
		// m_currTopLayer >= nodeTopLayer (see SimpleRemove
		// code to understand why).
		//

		for (int layer=0; layer < nodeTopLayer; layer++) {
			Node* pred = state.preds[layer];
			Node* curr = state.succs[layer];
			newNode->next[layer] = curr;
			pred->next[layer] = newNode;
		}

		// 4. Update currTopLayer if necessary:
		//
		if (nodeTopLayer > currTopLayer) {
			currTopLayer = nodeTopLayer;
		}

		newNode->fullyLinked = true;      // Linearization point
		membar();
		// unlock
		unlockPreds(highestLockedLayer, state.preds);
		return true;
	}
};


bool SkipListLTZ::remove(T v, ThreadLocal* threadLocal) {

	Node* nodeToDelete = NULL;
	int deleteTopLayer = -1;
	int retryCount = 0;
	int validCount = 0;
	State state;
	Node** preds = state.preds;
	Node** succs = state.succs;
	while (true) {
		state.layerFound = -1;
		// 1. Find node to be deleted and all its predecessors:
		//
		// 1.1 find node and first predecessor:
		Node* nodeFound = findNode(&state, v, threadLocal);
		if (nodeToDelete != NULL ||
				(nodeFound != NULL && nodeFound->fullyLinked &&
						state.layerFound == (nodeFound->height)-1 && !nodeFound->marked)) {

			// 1.2 If first time, lock and mark the node to be deleted:
			//
			if (nodeToDelete == NULL) {
				nodeToDelete = nodeFound;
				deleteTopLayer = state.layerFound;
				nodeToDelete->lock();
				if (nodeToDelete->marked) {
					nodeToDelete->unlock();
					return false;
				}
				nodeToDelete->marked = true;     // Linearization Point
				membar();
			}
			DASSERT(nodeToDelete->isLocked());
			// 2. Validate and lock predecessors:
			int highestLockedLayer = -1;
			bool valid = true;
			Node* pred = NULL;
			Node* prevPred = NULL;
			for (int layer = 0; valid && (layer <= deleteTopLayer); layer++) {
				pred = preds[layer];
				if (pred != prevPred) {
					pred->lock();
					highestLockedLayer = layer;
					prevPred = pred;
				}
				DASSERT(pred->next[layer]->value<=v);
				valid = valid && weakValidate(pred, nodeToDelete, layer);
			}
			if (!valid) {   // If validation failed, retry:
				unlockPreds(highestLockedLayer, preds);
#if defined(DEBUG)
				if (++validCount % LOOP_WARNING == 0) {
					printf("warning: remove() valid retry %d\n", validCount);
				}
#endif
				continue;
			}
			// 3. Unlink and update currTopLayer:
			for (int layer = deleteTopLayer; layer >= 0; layer--) {
				preds[layer]->next[layer] = nodeToDelete->next[layer];
				if (preds[layer] == &LSentinel &&
						preds[layer]->next[layer] == &RSentinel && layer!=0) {
					currTopLayer = layer-1;
					membar();
				}
			}
			nodeToDelete->unlock();
			manager.retire(nodeToDelete, threadLocal);
			unlockPreds(highestLockedLayer, preds);
			return true;
		} else {
			return false;
		}
	}
};
bool  SkipListLTZ::contains(T v, ThreadLocal* threadLocal) {
	int level;
	State state;
	state.pred = &LSentinel;
	Node* node = findNode(&state, v, threadLocal);
	bool result = node != NULL && node->fullyLinked && !node->marked;
	return result;
};

void SkipListLTZ::unlockPreds(int highestLockedLayer, Node** preds) {
	Node* prevUnlocked = NULL;
	for (int layer=0; layer<=highestLockedLayer; layer++) {
		if (prevUnlocked != preds[layer]) {
			preds[layer]->unlock();
			prevUnlocked = preds[layer];
		}
	}
}

bool SkipListLTZ::teleport(State* state, T v, ThreadLocal* threadLocal) {
	// first time?
	threadLocal->teleportLimit = DEFAULT_TELEPORT_DISTANCE;
	int distance = 0;
	Node* pred = state->pred;
	Node* curr;
	int layer = MAX_HEIGHT - 1;
	int layerFound = -1;
	// keep going until either we finish or the transaction range vanishes
	// transaction retry loop
	int retries = 10;
	while(--retries){
		threadLocal->teleports++;
		if (Set::xbegin(threadLocal) == _XBEGIN_STARTED) {
			// within transaction loop
			while (true) {
				curr = pred->next[layer];
				// layer traversal loop
				while (curr->value < v) {
					pred = curr;
					curr = curr->next[layer];
					if (distance++ > threadLocal->teleportLimit) {
						break;
					}
				}
				manager.record(pred, layer, threadLocal);
				manager.record(curr, layer, threadLocal);
				state->preds[layer] = pred;
				state->succs[layer] = curr;
				if (layerFound == -1 && v == curr->value) {
					layerFound = layer;
				}
				// did we finish the layer?
				if (curr->value >= v) {
					layer--;
				}
				// try to commit?
				if (distance > threadLocal->teleportLimit || layer < 0) {
					_xend();
					threadLocal->tCommitted++;
					threadLocal->teleportDistance += distance;
					threadLocal->teleportLimit++;
					state->layer = layer;
					state->layerFound = layerFound;
					state->pred = pred;
					state->curr = curr;
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
#define MIN_TEL_LIMIT 20
			threadLocal->teleportLimit=(threadLocal->teleportLimit>2*MIN_TEL_LIMIT)?threadLocal->teleportLimit/2:MIN_TEL_LIMIT;
		}
	}
	{
			// teleportation range hit bottom, save state, ask for fallback
			threadLocal->teleportLimit = DEFAULT_TELEPORT_DISTANCE;
			state->layer = layer;
			state->layerFound = layerFound;
			state->pred = pred;
			state->curr = curr;
			return false;
	}
};

void SkipListLTZ::fallback(State* state, T v, ThreadLocal* threadLocal) {
	threadLocal->fallback++;
	Node* pred = state->pred;
	Node* curr;
	int layer = state->layer;
	int layerFound = state->layerFound;
	while (layer >= 0) {
		curr = manager.read(&pred->next[layer], layer, threadLocal);
		while (curr->value < v) {
			pred = curr;
			curr = manager.read(&pred->next[layer], layer, threadLocal);
		}
		state->preds[layer] = pred;
		state->succs[layer] = curr;
		if (layerFound == -1 && v == curr->value) {
			layerFound = layer;
		}
		layer--;
	}
	state->layerFound = layerFound;
	state->pred = pred;
	state->curr = curr;
	state->layer = layer;
	return;
};
