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
	static SpinLock sgl;
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
	// helper functions:
	void unlock(Node* nodes[], int start, int stop);
	inline void unlock(Node* nodes[]) {unlock(nodes, 0, MAX_HEIGHT);};
	unsigned int xbegin(ThreadLocal* threadLocal);
	Node* findNode(Node* preds[],
			T v,
			ThreadLocal* threadLocal);
	void teleport(int layer, Node* preds[], T v, ThreadLocal* threadLocal);
};

#endif /* _SKIPLISTHTZ_H */
