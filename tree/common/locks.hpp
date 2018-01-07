///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2005, 2006, 2007, 2008, 2009
// University of Rochester
// Department of Computer Science
// All rights reserved.
//
// Copyright (c) 2009, 2010
// Lehigh University
// Computer Science and Engineering Department
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//    * Redistributions of source code must retain the above copyright notice,
//      this list of conditions and the following disclaimer.
//
//    * Redistributions in binary form must reproduce the above copyright
//      notice, this list of conditions and the following disclaimer in the
//      documentation and/or other materials provided with the distribution.
//
//    * Neither the name of the University of Rochester nor the names of its
//      contributors may be used to endorse or promote products derived from
//      this software without specific prior written permission.
//
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#ifndef LOCKS_HPP__
#define LOCKS_HPP__

#include "platform.hpp"

/**
 *  This is a bit ugly... the bet backoff parameters for SPARC and x86 differ.
 *
 *  [mfs] are we sure these values represent good choices in the real world?
 */
#if defined(STM_CPU_SPARC)
#define MAX_TATAS_BACKOFF 4096
#else // (STM_CPU_X86)
#define MAX_TATAS_BACKOFF 524288
#endif

/**
 *  Issue 64 nops to provide a little busy waiting
 */
inline void spin64()
{
    for (int i = 0; i < 64; i++)
        nop();
}

/**
 *  exponential backoff for TATAS locks
 */
inline void backoff(int *b)
{
    for (int i = *b; i; i--)
        nop();
    if (*b < MAX_TATAS_BACKOFF)
        *b <<= 1;
}

/**
 *  TATAS lock: test-and-test-and-set with exponential backoff
 */
typedef volatile uint32_t tatas_lock_t;

/**
 *  Slowpath TATAS acquire.  This performs exponential backoff
 */
inline int tatas_acquire_slowpath(tatas_lock_t* lock)
{
    int b = 64;
    do {
        backoff(&b);
    } while (tas(lock));
    return b;
}

/**
 *  Fastpath TATAS acquire.  The return value is how long we spent spinning
 */
inline int tatas_acquire(tatas_lock_t* lock)
{
    if (!tas(lock))
        return 0;
    return tatas_acquire_slowpath(lock);
}

inline void tatas_acq_silent(tatas_lock_t* lock){
	if(*lock==0) return;
	int b=64;
	do{
		backoff(&b);
		CFENCE;
	}while(*lock);
}
/**
 *  TATAS release: ordering is safe for SPARC, x86
 */
inline void tatas_release(tatas_lock_t* lock)
{
    CFENCE;
    *lock = 0;
}
class tatas_lock final{
	tatas_lock_t locked=0;
public:
	void acquire(){
		tatas_acquire(&locked);
	}
	void silent_acq(){
		tatas_acq_silent(&locked);
	}
	void release(){
		tatas_release(&locked);
	}
	bool isLocked(){
		return (bool)locked;
	}
	void acquire_tsx_unlocked(){
		//CFENCE; locked=1; CFENCE;
		locked=1;
	}
};

/**
 *  Ticket lock: this is the easiest implementation possible, but might not be
 *  the most optimal w.r.t. cache misses
 */
struct ticket_lock_t
{
    volatile uintptr_t next_ticket;
    volatile uintptr_t now_serving;
};

/**
 *  Acquisition of a ticket lock entails an increment, then a spin.  We use a
 *  counter to measure how long we spend spinning.
 */
inline int ticket_acquire(ticket_lock_t* lock)
{
    int ret = 0;
    uintptr_t my_ticket = faiptr(&lock->next_ticket);
    while (lock->now_serving != my_ticket)
        ret++;
    return ret;
}

/**
 *  Release the ticket lock
 */
inline void ticket_release(ticket_lock_t* lock)
{
    lock->now_serving += 1;
}

#if 0
/**
 *  Simple MCS lock implementation
 */
struct mcs_qnode_t
{
    volatile bool flag;
    volatile mcs_qnode_t* volatile next;
};

/**
 *  MCS acquire.  We count how long we spin, in order to detect very long
 *  delays
 */
inline int mcs_acquire(mcs_qnode_t** lock, mcs_qnode_t* mine)
{
    // init my qnode, then swap it into the root pointer
    mine->next = 0;
    mcs_qnode_t* pred = (mcs_qnode_t*)atomicswapptr(lock, mine);

    // now set my flag, point pred to me, and wait for my flag to be unset
    int ret = 0;
    if (pred != 0) {
        mine->flag = true;
        pred->next = mine;
        while (mine->flag) { ret++; } // spin
    }
    return ret;
}

/**
 *  MCS release
 */
inline void mcs_release(mcs_qnode_t** lock, mcs_qnode_t* mine)
{
    // if my node is the only one, then if I can zero the lock, do so and I'm
    // done
    if (mine->next == 0) {
        if (bcasptr(lock, mine, static_cast<mcs_qnode_t*>(NULL)))
            return;
        // uh-oh, someone arrived while I was zeroing... wait for arriver to
        // initialize, fall out to other case
        while (mine->next == 0) { } // spin
    }
    // other case: someone is waiting on me... set their flag to let them start
    mine->next->flag = false;
}
#endif

#endif // LOCKS_HPP__
