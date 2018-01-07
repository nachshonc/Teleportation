/**
 * hand-over-hand list
 * teleportation
 * memory management
 */
#ifndef _LISTHTZ_H
#define _LISTHTZ_H
#define null NULL
#include <limits.h>
#include "plat.h"
#include "SpinLock.h"
#include "List.h"
#include "ThreadLocal.h"
#include "SimpleManager.h"

typedef long T;

class ListHTZ : public List {

public:


public:
	SimpleManager manager;
	ListHTZ();
	bool add(T o, ThreadLocal* threadLocal);
	bool remove(T o, ThreadLocal* threadLocal);
	bool contains(T o, ThreadLocal* threadLocal);
	int size();
	bool sanity();
	void display();

private:
	Node LSentinel;
	Node RSentinel;
	// General helper functions:
	Node* teleport(Node* start, T v, ThreadLocal* threadLocal);
	unsigned int xbegin(ThreadLocal* threadLocal);

};

#endif /* _LISTHTZ_H */
