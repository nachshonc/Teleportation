/**
 * Lazy list
 * epoch-based
 */
#ifndef _LISTLE_HPP
#define _LISTLE_HPP
#define null NULL
#include <limits.h>
#include "plat.h"
#include "SpinLock.h"
#include "List.h"
#include "ThreadLocal.h"
#include "EpochManager.h"

class ListLE : public List {
	EpochManager epochManager;
public:
	ListLE();
	bool add(T o, ThreadLocal* threadLocal);
	bool remove(T o, ThreadLocal* threadLocal);
	bool contains(T o, ThreadLocal* threadLocal);
private:
	Node LSentinel;
	Node RSentinel;
#if defined(DEBUG)
	char buf[128];
#endif

	// General helper functions:
	bool validate(Node* pred, Node* curr);
};


#endif /* _LISTLE_H */
