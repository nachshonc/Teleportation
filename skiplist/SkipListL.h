/**
 * Lazy Skiplist
 * author: Maurice Herlihy
 */
#ifndef _SKIPLISTL_H
#define _SKIPLISTL_H
#include "SkipList.h"

class SkipListL: public SkipList {

public:
  static const int MAX_HEIGHT = 8;
public:
  Node LSentinel;
  Node RSentinel;
  int currTopLayer;
    
  SkipListL();
  bool add(T o, ThreadLocal* threadLocal);
  bool remove(T o, ThreadLocal* threadLocal);
  bool contains(T o, ThreadLocal* threadLocal);
  // findNode: looks for a given key.
  // If key in the list, returns the node containing
  // this key, and updates pred, succ, and level.
  // Otherwise returns null.
  //
private:
  Node* findNode(T o, Node* pred[], Node* succ[], int* levelFound);

  // General helper functions:

  bool validate(Node* pred, Node* curr, int layer);
  bool weakValidate(Node* pred, Node* curr, int layer);
  void unlockPreds(int highestLockedLayer, Node** predArr);
  bool lockPreds(int nodeTopLayer,
		 Node** predArr,
		 Node** currArr,
		 int* highestLockedLayer);
};

#endif /* _SKIPLISTL_H */
