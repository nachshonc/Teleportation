/*
 * simple storage manager for lists
 */

#ifndef _SIMPLEMANAGER_HPP
#define _SIMPLEMANAGER_HPP
class SimpleManager {
#if defined(DEBUG)
	char buf[128];
#endif
public:
	SimpleManager()
	{
	};

	List::Node* alloc(T v, ThreadLocal* threadLocal) {
#ifndef DISABLE_MM
		List::Node* node;
		if (threadLocal->free != NULL) {
			node = (List::Node*) threadLocal->free;
			threadLocal->free = node->free;
			*node =  List::Node(v);
			threadLocal->recycle++;
			return node;
		}
#endif
		threadLocal->allocs++;
		return new List::Node(v);
	};

	void retire(List::Node* node, ThreadLocal* threadLocal) {
#ifndef DISABLE_MM
		node->free  = (List::Node*) threadLocal->free;
		threadLocal->free = node;
#endif
	};

};
#endif /*  _SIMPLEMANAGER_HPP */
