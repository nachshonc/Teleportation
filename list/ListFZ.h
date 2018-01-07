/**
 * lock-free list with hazard pointers
 */
#ifndef _LISTF_H
#define _LISTF_H
#include <stdint.h>
#include <stdio.h>
#include <limits.h>
#include "plat.h"
#include "List.h"
#include "ThreadLocal.h"
#include "AtomicMarkedReference.h"
#include "HazardManager.h"

class ListFZ : public List {

public:

	HazardManager hazardManager;
public:
	Node LSentinel;
	Node RSentinel;
	Node* hazard[32];

	ListFZ();
	bool add(T o, ThreadLocal* threadLocal);
	bool remove(T o, ThreadLocal* threadLocal);
	bool contains(T o, ThreadLocal* threadLocal);
	int size() {		// for validation, not thread-safe
		int result = 0;
		Node* node = (Node*) LSentinel.aNext.getReference();
		while (node != &RSentinel) {
			if (!node->aNext.isMarked()) {
				result++;
			}
			node = (Node*) node->aNext.getReference();
		}
		return result;
	}

private:
#if defined(DEBUG)
	char buf[128];
#endif
	// General helper functions:
	void find(T v, Node** predPtr, Node** currPtr, ThreadLocal* threadLocal);

};

#endif /* _LISTF_H */
