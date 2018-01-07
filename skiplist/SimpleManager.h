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

  SkipList::Node* alloc(T v, int topLayer, ThreadLocal* threadLocal) {
    SkipList::Node* node;
    if (threadLocal->free != NULL) {
      node = (SkipList::Node*) threadLocal->free;
      threadLocal->free = node->free;
      *node = SkipList::Node(v, topLayer);
      threadLocal->recycle++;
      return node;
    } else {
      threadLocal->allocs++;
      return new SkipList::Node(v, topLayer);
    }
  };

  void retire(SkipList::Node* node, ThreadLocal* threadLocal) {
    node->free  = (SkipList::Node*) threadLocal->free;
    threadLocal->free = node;
  };

};
#endif /*  _SIMPLEMANAGER_HPP */
