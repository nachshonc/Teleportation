/**
 * Generic Set abstraction
 *
 * Maurice Herlihy
 pi * 20 August 2014
*/#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <pthread.h>
#include <iostream>
#include <algorithm>
#include <iterator>
#include <vector>
#include <sched.h>
#include <poll.h>
#include <alloca.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "ThreadLocal.h"
#include "xabort.h"
#ifndef _SET_H
#define	_SET_H

typedef long T;
class Set {
public:
  virtual bool add(T o, ThreadLocal* threadLocal) = 0;
  virtual bool remove(T o, ThreadLocal* threadLocal) = 0;
  virtual bool contains(T o, ThreadLocal* threadLocal) = 0;
  // validation only, not thread-safe
  virtual int size() = 0;
  virtual void display() = 0;
  virtual bool sanity() = 0;

  char* parseAbort(unsigned int status, char* buf) {
    if (status == _XBEGIN_STARTED) {
      sprintf(buf, "started");
    } else if (status & _XABORT_EXPLICIT) {
      switch (_XABORT_CODE(status)) {
      case XABORT_NOT_LINKED:
	sprintf(buf, "not linked");
	break;
      case XABORT_LOCKED:
	sprintf(buf, "locked");
	break;
      case XABORT_INVALID:
	sprintf(buf, "invalid");
	break;
      default:
	sprintf(buf, "explicit(%x)",_XABORT_CODE(status));
	break;
      }
    } else if (status & _XABORT_CONFLICT) {
      sprintf(buf,"conflict");
    } else if (status & _XABORT_DEBUG) {
      sprintf(buf,"debug");
    } else if (status & _XABORT_CAPACITY) {
      sprintf(buf,"capacity");
    } else if (status & _XABORT_NESTED) {
      sprintf(buf,"nested");
    } else if (status == 0) {
      sprintf(buf,"zero");
    }
    return buf;
  };

  unsigned int xbegin(ThreadLocal* threadLocal) {
    threadLocal->tStarted++;
    unsigned int status = _xbegin();
    if (status == _XBEGIN_STARTED) {
      return status;
    } else if (status & _XABORT_EXPLICIT) {
      threadLocal->aExplicit++;
      switch (_XABORT_CODE(status)) {
      case XABORT_NOT_LINKED:
	threadLocal->aNotLinked++;
	break;
      case XABORT_LOCKED:
	threadLocal->aLocked++;
	break;
      case XABORT_INVALID:
	threadLocal->aInvalid++;
	break;
      default:
	printf("error: unknown explicit code: %x\n",_XABORT_CODE(status));
	break;
      }
    } else if (status & _XABORT_CONFLICT) {
      threadLocal->aConflict++;
    } else if (status & _XABORT_DEBUG) {
      threadLocal->aDebug++;
    } else if (status & _XABORT_CAPACITY) {
      threadLocal->aCapacity++;
    } else if (status & _XABORT_NESTED) {
      threadLocal->aNested++;
    } else if (status == 0) {
      threadLocal->aUnknown++;
    }
    return status;
  }

};

#endif	/* _SET_H */
