/*
 * epoch-based storage management
 */

#ifndef _EPOCHMANAGER_HPP
#define _EPOCHMANAGER_HPP
class EpochManager {
#if defined(DEBUG)
  char buf[128];
#endif
public:
  typedef unsigned int uint;
  const static int EPOCH_COUNT = 3;
  const static int MAX_THREADS = 32;
  inline void membar() {
    __asm__ __volatile__ ("lock;addl $0,(%%rsp);" ::: "cc") ;
  };
  class ThreadEpoch {
  public:
    uint myEpoch;
    uint busy;
    List::Node* head[EPOCH_COUNT];
    List::Node* tail[EPOCH_COUNT];
    List::Node* free;
  };
  volatile uint globalEpoch = 1024;
  ThreadEpoch threadEpoch[MAX_THREADS];
  EpochManager() { };

  void enter(ThreadLocal* threadLocal) {
    ThreadEpoch* tEpoch = &threadEpoch[threadLocal->threadNum];
    tEpoch->busy = 1;
    uint epoch = globalEpoch;
    tEpoch->myEpoch = epoch;
    membar();
    int slot = (epoch - 2) % EPOCH_COUNT;
    if (tEpoch->free == NULL && tEpoch->head[slot] != NULL) {
      forceAgreement(epoch, threadLocal);
    }
    bool agree = tryAgreement(epoch, threadLocal);
    if (agree) {

      if (tEpoch->tail[slot] != NULL) {
 	tEpoch->tail[slot]->free = tEpoch->free;
 	tEpoch->free = tEpoch->head[slot];
 	tEpoch->head[slot] = NULL;
 	tEpoch->tail[slot] = NULL;
      }
      CAS64(&globalEpoch, epoch, epoch+1);
    }
  };
  
  void exit(ThreadLocal* threadLocal) {
    threadEpoch[threadLocal->threadNum].busy = 0;
    membar();
  }

  List::Node* alloc(T v, ThreadLocal* threadLocal) {
    ThreadEpoch* tEpoch = &threadEpoch[threadLocal->threadNum];
    List::Node* node;
    if (tEpoch->free != NULL) {
      node = tEpoch->free;
      tEpoch->free = node->free;
      threadLocal->recycle++;
      *node = List::Node(v);
    } else {
      threadLocal->allocs++;
      node = new List::Node(v);
    }
    return node;
  }

  void retire(List::Node* node, ThreadLocal* threadLocal) {
    ThreadEpoch* tEpoch = &threadEpoch[threadLocal->threadNum];
    int slot = tEpoch->myEpoch % EPOCH_COUNT;
    node->free = tEpoch->head[slot];
    tEpoch->head[slot] = node;
    if (tEpoch->tail[slot] == NULL) {
      tEpoch->tail[slot] = node;
    }
  }


  /*
   * make one attempt to wait until everyone catches up
   */
  bool tryAgreement(uint global, ThreadLocal* threadLocal) {
    for (int i = 0; i < threadLocal->numThreads; i++) {
      ThreadEpoch* anEpoch = &threadEpoch[i];
      if (anEpoch->busy && anEpoch->myEpoch < global) {
	return false;
      }
    }
    return true;
  };
  /*
   * force wait until everyone catches up
   */
  void forceAgreement(uint global, ThreadLocal* threadLocal) {
    while (! tryAgreement(global, threadLocal)) {
      Pause();
    }
  };

};

#endif /* _EPOCHMANAGER_HPP */
