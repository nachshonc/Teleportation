/**
 * Lazy Skiplist
 * author: Maurice Herlihy
 */
#ifndef _SKIPLISTHTZ_H
#define _SKIPLISTHTZ_H
#include "SkipList.h"
#include "SimpleManager.h"

class SkipListHTZ: public SkipList {
public:
	static const int DEFAULT_TELEPORT_DISTANCE = 512;
	SkipListHTZ();
	bool add(T o, ThreadLocal* threadLocal);
	bool remove(T o, ThreadLocal* threadLocal);
	bool contains(T o, ThreadLocal* threadLocal);
	// storage
	SimpleManager manager;

private:
	Node LSentinel;
	Node RSentinel;
#if defined(DEBUG)
	char buf[128];		// debugging messages
#endif
	volatile int currTopLayer;
	// helper functions:
	void unlock(Node* nodes[], int start, int stop);
	inline void unlock(Node* nodes[]) {unlock(nodes, 0, MAX_HEIGHT);};
	unsigned int xbegin(ThreadLocal* threadLocal);
	class State{
	public:
		int layer = 0;
		Node* preds[MAX_HEIGHT];
		bool validate(T v) {
			char buffer[256];
			bool ok = true;
			int low = (layer > -1) ? layer : 0;
			for (int i = MAX_HEIGHT-1; i >= low ; i--) {
				if (preds[i] == NULL) {
					printf("error:\tpreds[%d] is NULL\n", i);
					ok = false;
				} else if (! preds[i]->isLocked()) {
					printf("error:\tpreds[%d] = %s should be locked\n",
							i,
							preds[i]->toString(buffer));
					ok = false;
				} else if (preds[i]->value >= v) {
					printf("error:\tpreds[%d]->value = %d >= %d\n",
							i,
							preds[i]->value,
							v);
					ok = false;
				}
				if (i > layer && preds[i]->next[i]->value < v) {
					printf("error:\tpreds[%d]->next[%d]->value = %d < %d\n",
							i, i,
							preds[i]->next[i]->value,
							v);
					ok = false;
				}
			}
			if (!ok) exit(0);
		};
	};
	Node* findNode(State* state,
			T v,
			ThreadLocal* threadLocal);
	bool teleport(State* state, T v, ThreadLocal* threadLocal);
	void fallback(State* state, T v, ThreadLocal* threadLocal);
};

#endif /* _SKIPLISTHTZ_H */
