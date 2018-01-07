/**
 * Lazy list
 * hazard pointers
 */
#include "ListLZ.h"

ListLZ::ListLZ():
  LSentinel(LONG_MIN),
  RSentinel(LONG_MAX),
  hazardManager(2) {
  LSentinel.next = (List::Node*) &RSentinel;
};

bool ListLZ::validate(List::Node* pred, List::Node* curr) {
  return (!pred->marked) && (!curr->marked) && (pred->next == curr);
};

bool ListLZ::add(T v, ThreadLocal* threadLocal) {
  List::Node* pred;
  List::Node* curr;
  bool result = false;
  int retryCount = 0;
  while (true) {
    curr = findNode(v, threadLocal, &pred);

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
      List::Node* node = hazardManager.alloc(v, threadLocal);
      node->marked=0; 
      node->next = curr;
      pred->next = node;
      result = true;
      break;
    }
#if defined(debug)
    if (++retryCount % 10000 == 0) {
      printf("add retry(s) %d\n", retryCount);
    }
#endif
  }
  pred->unlock();
  curr->unlock();
  return result;
};


bool ListLZ::remove(T v, ThreadLocal* threadLocal) {
  List::Node* pred;
  List::Node* curr;
  bool result = false;
  int retryCount = 0;

  while (true) {
    curr = findNode(v, threadLocal, &pred);

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
      hazardManager.retire(curr, threadLocal);
      result = true;
      break;
    } else {
      result = false;
      break;
    }
#if defined(DEBUG)
    if (++retryCount % 10000 == 0) {
      printf("warning:\tadd retry %d\n", retryCount);
    }
#endif
  } 
  pred->unlock();
  curr->unlock();

  return result;
};

bool ListLZ::contains(T v, ThreadLocal* threadLocal) {
  List::Node *node = findNode(v, threadLocal);
  return node->value == v && !node->marked;
};

List::Node *ListLZ::findNode(T v, ThreadLocal* threadLocal,
    List::Node **prev /* = NULL */) {
  List::Node *node;
  List::Node *prevNode;
  bool restart;

  do {
    restart = false;
    prevNode = &LSentinel;
    node = hazardManager.read(&(LSentinel.next), threadLocal);
    while (node->value < v) {
      prevNode = node;
      node = hazardManager.read(&(node->next), threadLocal);
      if (prevNode->marked) { restart = true; break; }
    }
  } while (restart);

  if (prev) { *prev = prevNode; }

  return node;
}

