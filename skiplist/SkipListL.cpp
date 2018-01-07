/**
 * Lazy Skiplist
 * author: Maurice Herlihy
 */
#include "SkipListL.h"
#if defined(DEBUG)
#define LOOP_WARNING 1000000
#endif

unsigned int SkipList::randomSeed = rand();

// findNode: looks for a given key.
// If key in the list, returns the node containing
// this key, and update appropriate location in predArr. 
// Otherwise returns null.
//
SkipListL::Node* SkipListL::findNode(T v,
			   Node* preds[],
			   Node* succs[],
			   int* level) {
  int levelFound = -1;
  Node* pred = &LSentinel;
  Node* curr = NULL;
  for( int layer = MAX_HEIGHT-1; layer >= 0; layer--) {
    curr = pred->next[layer];
    while (v > curr->value) {
      pred = curr;
      curr = pred->next[layer];
    }
    if (preds != NULL) {
      preds[layer] = pred;
    }
    succs[layer] = curr;
    if (levelFound == -1 && v == curr->value) {
      levelFound = layer;
    }
  }
  *level = levelFound;
  if (v == curr->value) {
    return curr;
  } else {
    return NULL;
  }
};

SkipListL::SkipListL():
  LSentinel(LONG_MIN, MAX_HEIGHT),
  RSentinel(LONG_MAX, MAX_HEIGHT) {
  for (int i = 0; i < MAX_HEIGHT; i++) {
    LSentinel.next[i] = (Node*) &RSentinel;
  };
  currTopLayer = 0;
};

bool SkipListL::validate(Node* pred, Node* curr, int layer) {
  return (!pred->marked) && (!curr->marked) && (pred->next[layer]==curr);
};

bool SkipListL::weakValidate(Node* pred, Node* curr, int layer) {
  return (!pred->marked) && (pred->next[layer]==curr);
};


bool SkipListL::add(T v, ThreadLocal* threadLocal) {
  int nodeTopLayer = randomLevel();
  Node* predArr[MAX_HEIGHT];
  Node* currArr[MAX_HEIGHT];
  int markedCount = 0;
  int linkedCount = 0;
  int validCount = 0;
  unsigned int backoff;
  struct timespec timeout;

  while (true) {
    int highestLockedLayer = -1;
    Node* pred = &LSentinel;
    bool valid = true;
    int level;
    //
    // Find neighbors
    //
    Node* node = findNode(v, predArr, currArr, &level);
    if (node != NULL) {
      if (!node->marked) {
	int spinCount = 0;
	while (! node->fullyLinked) {
#if defined(DEBUG)
	  if (linkedCount % LOOP_WARNING == 0) {
	    printf("warning: add() fully linked retry %d\n", linkedCount);
	  }
#endif
	} // spin
	return false;
      }
#if defined(DEBUG)
      if (++markedCount % LOOP_WARNING == 0) {
	printf("warning: add() marked retry %d\n", markedCount);
      }
#endif
      Pause();
      continue;
    }

    // 2. Validate and lock predecessors:
    //
    Node* prevPred = NULL;
    for (int layer = 0; valid && (layer < nodeTopLayer); layer++) {
      pred = predArr[layer];
      Node* curr = currArr[layer];
      if (pred != prevPred) {
	pred->lock();
	highestLockedLayer = layer;
	prevPred = pred;
      }
      valid = valid && validate(pred, curr, layer);
    }                    
    if (!valid) {   // If validation failed, retry:
      // unlock predecessors
      unlockPreds(highestLockedLayer, predArr);
//       if (backoff > 5000) {
// 	timeout.tv_sec = backoff / 5000;
// 	timeout.tv_nsec = (backoff % 5000) * LOOP_WARNING0;
// 	nanosleep(&timeout, NULL);
//       }
//       backoff *= 2;
#if defined(DEBUG)
      if (++validCount % LOOP_WARNING == 0) {
	printf("warning: add() valid retry %d\n", validCount);
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
    Node* newNode = new Node(v, nodeTopLayer);
    for (int layer=0; layer < nodeTopLayer; layer++) {
      pred = predArr[layer];
      Node* curr = currArr[layer];
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
    unlockPreds(highestLockedLayer, predArr);
    return true;
  }
};


bool SkipListL::remove(T v, ThreadLocal* threadLocal) {
  Node* nodeToDelete = NULL;
  int deleteTopLayer = -1;
  Node* predArr[MAX_HEIGHT];
  Node* currArr[MAX_HEIGHT];
  int layerFound;
  int retryCount = 0;
  int validCount = 0;

  while (true) {
    // 1. Find node to be deleted and all its predecessors:
    //
    // 1.1 find node and first predecessor:
    Node* nodeFound = findNode(v, predArr, currArr, &layerFound);
    if (nodeToDelete != NULL ||
	(nodeFound != NULL && nodeFound->fullyLinked && 
	 layerFound == (nodeFound->height)-1 && !nodeFound->marked)) {

      // 1.2 If first time, lock and mark the node to be deleted:
      //
      if (nodeToDelete == NULL) {
	nodeToDelete = nodeFound;
	deleteTopLayer = layerFound;
	nodeToDelete->lock();
	if (nodeToDelete->marked) {
	  nodeToDelete->unlock();
	  return false;
	}
	nodeToDelete->marked = true;     // Linearization Point
      }
      int highestLockedLayer = -1;
      bool valid = true;
      // 2. Validate and lock predecessors:
      Node* pred = NULL;
      Node* prevPred = NULL;
      for (int layer = 0; valid && (layer <= deleteTopLayer); layer++) {
	pred = predArr[layer];
	if (pred != prevPred) {
	  pred->lock();
	  highestLockedLayer = layer;
	  prevPred = pred;
	}
	valid = valid && weakValidate(pred, nodeToDelete, layer);
      }                    
      if (!valid) {   // If validation failed, retry:
	unlockPreds(highestLockedLayer, predArr);
#if defined(DEBUG)
	if (++validCount % LOOP_WARNING == 0) {
	  printf("warning: remove() valid retry %d\n", validCount);
	}
#endif
	continue;
      }
      // 3. Unlink and update currTopLayer:
      for (int layer = deleteTopLayer; layer >= 0; layer--) {
	predArr[layer]->next[layer] = nodeToDelete->next[layer];
	if (predArr[layer] == &LSentinel &&
	    predArr[layer]->next[layer] == &RSentinel && layer!=0) {
	  currTopLayer = layer-1;
	  membar();
	}
      }

      nodeToDelete->unlock();
      unlockPreds(highestLockedLayer, predArr);
      return true;
    } else {
      return false;
    }
  } 
};
bool  SkipListL::contains(T o, ThreadLocal* threadLocal) {
  Node* predArr[MAX_HEIGHT];
  Node* currArr[MAX_HEIGHT];
  int level;
  Node* node = findNode(o, predArr, currArr, &level);
  return (node!=NULL && node->fullyLinked && !node->marked);
};

void SkipListL::unlockPreds(int highestLockedLayer, Node** predArr) {
  Node* prevUnlocked = NULL;
  for (int layer=0; layer<=highestLockedLayer; layer++) {          
    if (prevUnlocked != predArr[layer]) {
      predArr[layer]->unlock();
      prevUnlocked = predArr[layer];
    }
  }
}
