/*
 * hazard pointer-based storage management
 * for lists
 */


#ifndef _HAZARDMANAGER_HPP
#define _HAZARDMANAGER_HPP
#include "AtomicMarkedReference.h"
#include "List.h"
#include <assert.h>

#define PADDING_SIZE (2 * 64)

#define SCAN_TRIGGER (64)

#include <sys/mman.h>
#define     RWX          (PROT_READ|PROT_WRITE|PROT_EXEC)
static int getPageSize() {
	return sysconf (_SC_PAGESIZE);
}

static void *getPage(int pgsz) {
	return mmap (NULL, pgsz, RWX, MAP_ANON|MAP_PRIVATE|MAP_NORESERVE, -1, 0LL);
}
static inline void hStore(void** addr, void* hazard) {
	*addr = hazard;
};

class HazardManager {
#if defined(DEBUG)
	char buf[128];
#endif
public:

	const int perThread=4, MASK=perThread-1;
	const static int MAX_THREADS = 32;
	List::Node** hazard;

	HazardManager(int numHPsPerThread) //: perThread(numHPsPerThread)
	{
		assert(numHPsPerThread<=perThread);
		uint32_t size =
				(sizeof(List::Node*) * perThread + PADDING_SIZE) * MAX_THREADS;
		int pgsz = getPageSize();
#if !defined(PAGE_PER_THREAD)
		// make sure we allocate chunk of memory that is multiple of page size
		uint32_t hazardTableSize = size % pgsz ? (size / pgsz + 1) * pgsz : size;
		// update page size, as this is the granularity we will use for mprotect
		pgsz = hazardTableSize; //nachshon: don't understand this assignment.
#else
		// do no worry about padding in this case -- each thread gets a
		// separate page, and we do not expect the number of hazard pointers to be
		// huge to fill all of it
		uint32_t hazardTableSize = pgsz * MAX_THREADS;
#endif
		void *hazardTable = getPage(hazardTableSize);
		memset(hazardTable, 0, hazardTableSize);
		hazard = (List::Node **)hazardTable;
	};

	inline void membar() {
		__asm__ __volatile__ ("lock;addl $0,(%%rsp);" ::: "cc","memory") ;
	};

	inline int getFirstHazardSlot(uint32_t threadID) {
		return (PADDING_SIZE / sizeof(List::Node *) + perThread) * threadID;
	}

	inline int getHazardSlot(ThreadLocal* threadLocal) {
		return getFirstHazardSlot(threadLocal->threadNum) +
				(threadLocal->clock & (MASK));
	}

	/*
	 * announce new hazard at end of teleportation
	 * transaction-safe
	 */
	void record(List::Node* node, ThreadLocal* threadLocal ) {
		threadLocal->clock++;
		int slot = getHazardSlot(threadLocal);

		hazard[slot] = node;
	}

	/*
	 * read pointer, mark as new hazard
	 */
	inline List::Node* read(List::Node* volatile * object, ThreadLocal* threadLocal) {
#ifndef DISABLE_MM
		threadLocal->clock++;
		return reread(object, threadLocal);
#else // leaky or optimistic
		return *object;
#endif // DISABLE_MM
	}
	/*  * read pointer, overwrite last hazard   */
	inline List::Node* reread(List::Node* volatile * object, ThreadLocal* threadLocal) {
#ifndef DISABLE_MM
		int slot = getHazardSlot(threadLocal);
		while (true) {
			List::Node* read = *object;
			hStore((void**) &hazard[slot], read);
			membar();
			List::Node* reread = *object;
			if (read == reread) {
				return read;
			}
		}
#else
		return *object;
#endif
	}
	List::Node* read(AtomicMarkedReference* object, ThreadLocal* threadLocal) {
#ifndef DISABLE_MM
		threadLocal->clock = (threadLocal->clock + 1) % perThread;
		return reread(object, threadLocal);
#else
		return (List::Node*)(void*) (*object).getReference();
#endif
	};
	List::Node* reread(AtomicMarkedReference* object, ThreadLocal* threadLocal) {
#ifndef DISABLE_MM
		int slot = getHazardSlot(threadLocal);
		while (true) {
			List::Node* read = (List::Node*) (*object).getReference();
			hStore((void**)&hazard[slot], read); //hazard[slot].pointer[threadLocal->clock] = read;
			membar();
			List::Node* reread = (List::Node*) (*object).getReference();
			if (read == reread) {
				return read;
			}
		}
#else
		return (List::Node*)(void*)(*object).getReference();
#endif
	};
	List::Node* alloc(T v, ThreadLocal* threadLocal) {
#ifdef DISABLE_MM
		return new List::Node(v);
#else
		List::Node* node;
		if (threadLocal->free != NULL) {
			node = (List::Node*) threadLocal->free;
			threadLocal->free = node->free;
			*node = List::Node(v);
			threadLocal->recycle++;
			return node;
		} else if (threadLocal->numRetired >= SCAN_TRIGGER) {
			threadLocal->scans++;
			List::Node* stillRetired = NULL;
			int tid = threadLocal->threadNum;
			int numStillRetired = 0;
			for (int i = 0; i < threadLocal->numThreads && threadLocal->retired; i++) {
				// no need to scan my own HPs
				if (i == tid) continue;
				int startIndex = getFirstHazardSlot(i);
				for (int j = 0; j < this->perThread; j++) {
					List::Node **prev = (List::Node**)&threadLocal->retired;
					for (node = (List::Node*) threadLocal->retired; node != NULL;) {
						List::Node *next = node->free;
						if (hazard[startIndex + j] == node) {
							// this node is hazardous - move it to 'stillRetired' list
							node->free = stillRetired;
							stillRetired = node;
							*prev = next;
							numStillRetired++;
						} else {
							prev = &node->free;
						}
						node = next;
					}
				}
			}
			// nodes that stay in 'retired' list after scan are free
			threadLocal->free = threadLocal->retired;
			// nodes that are moved to 'stillRetired' should be kept
			threadLocal->retired = stillRetired;
			threadLocal->numRetired = numStillRetired;
			if (threadLocal->free != NULL) {
				node = (List::Node*) threadLocal->free;
				threadLocal->free = node->free;
				*node = List::Node(v);
				threadLocal->recycle++;
				return node;
			} else {
				threadLocal->allocs++;
				return new List::Node(v);
			}
		} else {
			threadLocal->allocs++;
			return new List::Node(v);
		}
#endif // USE_HPs
	};

	void retire(List::Node* node, ThreadLocal* threadLocal) {
#ifndef DISABLE_MM
		node->free = (List::Node*) threadLocal->retired;
		threadLocal->retired = node;
		threadLocal->numRetired++;
#elif defined(OPTIMISTIC)
		delete node;
#endif // USE_HPs
	};
};
#endif /*  _HAZARDMANAGER_HPP */
