/*
 * hazard pointer-based storage management
 * for skiplists
 */
#ifndef _HAZARDMANAGER_H
#define _HAZARDMANAGER_H
#include "AtomicMarkedReference.h"
#include "SkipList.h"
#include <assert.h>
#define MAX_PER_LEVEL 4
#define MAX_THREADS 32
class HazardManager {
#if defined(DEBUG)
	char buf[128];
#endif
	class HazardPointer {
	public:
		ub4 align1[16]; // make each entry 128 bytes to ensure separate cache line
		SkipList::Node* pointer[SkipList::MAX_HEIGHT * MAX_PER_LEVEL];
	} __attribute__ ((aligned((64)))) ;
public:

	int perThread;
	HazardPointer* hazard;
	HazardManager(int size) :
		perThread(size),
		hazard(new HazardPointer[MAX_THREADS * perThread])
	{
		if (perThread > SkipList::MAX_HEIGHT * MAX_PER_LEVEL) {
			printf("Hazard manager %d size exceeds %d\n", perThread, SkipList::MAX_HEIGHT * MAX_PER_LEVEL);
			exit(0);
		}
	}
	inline void membar() {
		__asm__ __volatile__ ("lock;addl $0,(%%rsp);" ::: "cc") ;
	};

	/*
	 * announce new hazard at end of teleportation
	 * transaction-safe
	 */
	void record(SkipList::Node* node, int level, ThreadLocal* threadLocal ) {
		threadLocal->clock = (threadLocal->clock + 1) % MAX_PER_LEVEL;
		int tid = threadLocal->threadNum;
		int hslot = MAX_PER_LEVEL*level + threadLocal->clock;
		hazard[tid].pointer[hslot]=node;
	}

	/* read pointer, mark as new hazard   */
	inline SkipList::Node* read(SkipList::Node** object, int level, ThreadLocal* threadLocal) {
		threadLocal->clock = (threadLocal->clock + 1) % MAX_PER_LEVEL;
		return reread(object, level, threadLocal);
	}

	/* read pointer, overwrite last hazard */
	inline SkipList::Node* reread(SkipList::Node** object, int level, ThreadLocal* threadLocal) {
		int tid = threadLocal->threadNum;
		int hslot = MAX_PER_LEVEL*level + threadLocal->clock;
		while (true) {
			SkipList::Node* read = *object;
			hazard[tid].pointer[hslot] = read;
			membar();
			SkipList::Node* reread = *object;
			if (read == reread) {
				return read;
			}
		}
	};
	SkipList::Node* alloc(T v, int topLayer, ThreadLocal* threadLocal) {
		SkipList::Node* node;
		if (threadLocal->free != NULL) {
			node = (SkipList::Node*) threadLocal->free;
			threadLocal->free = node->free;
			node->set(v, topLayer);
			node->change_state(SkipList::Node::STATE_FREE, SkipList::Node::STATE_ALLOC);
			threadLocal->recycle++;
			return node;
		} else if (threadLocal->retired != NULL) {
			SkipList::Node* stillRetired = NULL;
			for (node = (SkipList::Node*) threadLocal->retired; node != NULL; node = node->free) {
				bool nodeIsFree = true;
				for (int i = 0; i < threadLocal->numThreads; i++) {
					for (int j = 0; j < perThread; j++) {
						if (hazard[i].pointer[j] == node) {
							DASSERT(node!=stillRetired);
							node->free = stillRetired;
							stillRetired = node;
							goto nextNode;
						}
					}
				}
				//if we arrived here then the node can be free. Not protected by HP.
				DASSERT(threadLocal->free != node);
				node->change_state(SkipList::Node::STATE_RETIRE, SkipList::Node::STATE_FREE);
				node->free = (SkipList::Node*) threadLocal->free;
				threadLocal->free=node;
				nextNode:	node->change_state(SkipList::Node::STATE_RETIRE, SkipList::Node::STATE_RETIRE);
			}
			threadLocal->retired = stillRetired;
			if (threadLocal->free != NULL) {
				node = (SkipList::Node*) threadLocal->free;
				threadLocal->free = node->free;
				node->set(v, topLayer);
				node->change_state(SkipList::Node::STATE_FREE, SkipList::Node::STATE_ALLOC);
				threadLocal->recycle++;
				return node;
			} else {
				threadLocal->allocs++;
				SkipList::Node *res = new SkipList::Node(v, topLayer);
				return res;
			}
		} else {
			threadLocal->allocs++;
			SkipList::Node *res = new SkipList::Node(v, topLayer);
			return res;
		}
	};

	void retire(SkipList::Node* node, ThreadLocal* threadLocal) {
		DASSERT(node != threadLocal->retired);
		node->change_state(SkipList::Node::STATE_ALLOC, SkipList::Node::STATE_RETIRE);
		node->free = (SkipList::Node*) threadLocal->retired;
		threadLocal->retired = node;
	};
};
#endif /*  _HAZARDMANAGER_H */
