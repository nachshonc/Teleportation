/**
 * hand-over-hand list
 * memory management
 */
#ifndef _LISTHZ_H
#define _LISTHZ_H
#define null NULL
#include <limits.h>
#include "plat.h"
#include "SpinLock.h"
#include "List.h"
#include "ThreadLocal.h"
#include "SimpleManager.h"

typedef long T;

class ListHZ : public List {

public:
	SimpleManager manager;
	ListHZ();
	bool add(T o, ThreadLocal* threadLocal);
	bool remove(T o, ThreadLocal* threadLocal);
	bool contains(T o, ThreadLocal* threadLocal);
	int size();
	bool sanity();
	void display();

private:
	Node LSentinel;
	Node RSentinel;

};

#endif /* _LISTHZ_H */
