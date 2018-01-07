/**
 * hand-over-hand skip list
 * no memory management
 */
#include "SkipListH.h"

unsigned int SkipList::randomSeed = rand();

void SkipListH::find(T v, Node* preds[], ThreadLocal* threadLocal) {
  Node* pred = &LSentinel;
  pred->lock();
#if defined(DEBUG)
  printf("SkipListH::find:\tjust locked LSentinel= %s\n",
	 pred->toString(buf));
#endif
  // At each loop start, pred locked, pred->value < v
  for (int layer = MAX_HEIGHT-1; layer >=0; layer--) {
    // lock layer's first node
    Node* curr = pred->next[layer];
    curr->lock();
    // At each loop start and finish, pred and curr both locked
#if defined(DEBUG)
    if (! curr->isLocked()) {
      printf("error:\tSkipListH::find:\tbefore loop layer %d, curr = %s not locked\n",
	     layer,
	     curr->toString(buf));
      exit(0);
    }
    if (! pred->isLocked()) {
      printf("error:\tSkipListH::find:\tbefore loop layer %d, pred = %s not locked\n",
	     layer,
	     pred->toString(buf));
      exit(0);
    }
#endif
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
      printf("error:\tSkipListH::find:\tlayer %d, curr = %s not locked\n",
	     layer,
	     curr->toString(buf));
      exit(0);
    }
    if (! pred->isLocked()) {
      printf("error:\tSkipListH::find:\tlayer %d, pred = %s not locked\n",
	     layer,
	     pred->toString(buf));
      exit(0);
    }
    if (! preds[layer]->isLocked()) {
      printf("error:\tSkipListH::find:\t preds[%d] = %s not locked\n",
	     layer, preds[layer]->toString(buf));
      exit(0);
    }
#endif
    curr->unlock();
  }
}

SkipListH::SkipListH():
  LSentinel(LONG_MIN, MAX_HEIGHT),
  RSentinel(LONG_MAX, MAX_HEIGHT) {
  for (int i = 0; i < MAX_HEIGHT; i++) {
    LSentinel.next[i] = (Node*) &RSentinel;
  };
  currTopLayer = 0;
};

bool SkipListH::add(T v, ThreadLocal* threadLocal) {
  bool result;
  Node* preds[MAX_HEIGHT];
  find(v, preds, threadLocal);
  if (preds[0]->next[0]->value == v) {
    result = false;
  } else {
    Node* node = manager.alloc(v, threadLocal);
    for (int layer = 0; layer < node->height; layer++) {
      node->next[layer] = preds[layer]->next[layer];
      preds[layer]->next[layer] = node;
#if defined(DEBUG)
      if (!preds[layer]->isLocked()) {
	printf("error:\tadd preds[%d] not locked\n", layer);
	exit(0);
      }
#endif
    }
    result = true;
  }
  unlock(preds);
  return result;
};

bool SkipListH::remove(T v, ThreadLocal* threadLocal) {
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
#if defined(DEBUG)
      if (!preds[layer]->isLocked()) {
	printf("error:\tadd preds[%d] not locked\n", layer);
	exit(90);
      }
#endif
    }
    manager.retire(nodeToDelete, threadLocal);
    result = true;
  }
  unlock(preds);
  return result;
};

bool  SkipListH::contains(T v, ThreadLocal* threadLocal) {
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

int SkipListH::size() {		// for validation, not thread-safe
  int result = 0;
  Node* node = LSentinel.next[0];
  while (node != &RSentinel) {
    result++;
    node = node->next[0];
  }
  return result;
}

void SkipListH::display() {		// for validation, not thread-safe
  Node* node = LSentinel.next[0];
  char buf[128];
  while (node != &RSentinel) {
    printf("%s\n", node->toString(buf));
    node = node->next[0];
  }
}

bool SkipListH::sanity() {
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

void SkipListH::unlock(Node* nodes[], int start, int stop) {
  Node* lastUnlocked = NULL;
  for (int layer = start; layer < stop; layer++) { // back out
    if (nodes[layer] != lastUnlocked) {
#if defined(DEBUG)
      if (!_xtest()) {
	if (!nodes[layer]->isLocked()) {
	  printf("SkipListH::unlock(%d, %d) layer %d node %s already unlocked\n",
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
