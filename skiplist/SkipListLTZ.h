/***
 * Lazy skip list
 * hazard pointers
 * teleportaton
 */
#ifndef _SKIPLISTLTZ_H
#define _SKIPLISTLTZ_H
#include "SkipList.h"
#include "HazardManager.h"

class SkipListLTZ: public SkipList {

public:
  static const int DEFAULT_TELEPORT_DISTANCE = 512;

  HazardManager manager;
  int currTopLayer;
    
  SkipListLTZ();
  bool add(T o, ThreadLocal* threadLocal);
  bool remove(T o, ThreadLocal* threadLocal);
  bool contains(T o, ThreadLocal* threadLocal);

private:
  class State{
  public:
    int layer = 0;
    int layerFound = -1;
    Node* pred;
    Node* curr;
    Node* preds[MAX_HEIGHT];
    Node* succs[MAX_HEIGHT];
    char* toString(char* buf) {
      sprintf(buf, "State[layer: %d\n\tlayerfound: %d\n\tpred: %p\n\t curr: %p]",
	      layer, layerFound, pred, curr);
      return buf;
    }
  };

  Node* findNode(State* state,
		 T v,
		 ThreadLocal* threadLocal);
  // General helper functions:
  bool teleport(State* state, T v, ThreadLocal* threadLocal);
  void fallback(State* state, T v, ThreadLocal* threadLocal);
  bool validate(Node* pred, Node* curr, int layer);
  bool weakValidate(Node* pred, Node* curr, int layer);
  void unlockPreds(int highestLockedLayer, Node** preds);
  bool lockPreds(int nodeTopLayer,
		 Node** preds,
		 Node** currs,
		 int* highestLockedLayer);
};

#endif /* _SKIPLISTLTZ_H */
