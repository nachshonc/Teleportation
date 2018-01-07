/*
 *  Created on: Jan 18, 2015
 *      Author: nachshonc
 *  Hand Over Hand Tree with Teleportation
 */

#ifndef TREEHT_HPP_
#define TREEHT_HPP_
#include "tree.hpp"
#include "Set.h"
#include "common/locks.hpp"
#include <string>
#include <iostream>
#include <assert.h>
#include <immintrin.h>
#include <rtmintrin.h>
using namespace std;
#define LOCK_HELD XABORT_LOCKED
extern int retires;
#define START_TX() Set::xbegin(threadLocal)
#define TEL_DIST(threadLocal, dist) ((threadLocal)->teleportDistance+=(dist))
#define FALLBACK(threadLocal) ((threadLocal)->fallback++)
#define START_OP(threadLocal) ((threadLocal)->ops++)
#define START_TEL(threadLocal) ((threadLocal)->teleports++)
class TreeHT: public Set {
private:
public:
	struct node {
		unsigned key;
		node *left, *right;
		void *obj;
		tatas_lock lock;
	};
	node *head;
	///------------------------------------------------------------------------search
	node* teleport_cur(node *start, T key, ThreadLocal *threadLocal) {
		int status;
		(void) status;
		assert(start==NULL || start->lock.isLocked());
		for (int i = 0; i < retries; ++i) {
			if (START_TX() == _XBEGIN_STARTED) {
				int limit = threadLocal->teleportLimit;
				node *cur = (start == NULL) ? head : start;
				int res = false;
				while (cur != NULL && --limit) {
					unsigned curkey = cur->key;
					if (key == curkey) {
						cur->lock.acquire_tsx_unlocked();
						res = true;
						break;
					}
					if (key < curkey)
						cur = cur->left;
					else
						cur = cur->right;
					if (cur != NULL && cur->lock.isLocked())
						_xabort(LOCK_HELD);
				}
				if (cur != NULL && limit == 0)
					cur->lock.acquire_tsx_unlocked();
				_xend();
				TEL_DIST(threadLocal, threadLocal->teleportLimit-limit);
				threadLocal->teleportLimit += 1;
				if (start != NULL && start == cur) {
					printf("start=%p, cur=%p\n", start, cur);
					assert(0);
				}
				assert(start==NULL || start->lock.isLocked());
				if (start != NULL)
					start->lock.release();
				assert(cur==NULL || cur->lock.isLocked());
				return cur;
			}
			threadLocal->teleportLimit=(threadLocal->teleportLimit>MIN_TEL_LIMIT*2)?threadLocal->teleportLimit/2:MIN_TEL_LIMIT;
		}
		FALLBACK(threadLocal);
		if (start == NULL) {
			start = head;
			start->lock.acquire();
			return start;
		}
		node *next;
		if (key < start->key)
			next = start->left;
		else
			next = start->right;
		if (next == NULL) {
			start->lock.release();
			return NULL;
		}
		next->lock.acquire();
		start->lock.release();
		assert(next==NULL || next->lock.isLocked());
		return next;
	}
	bool contains(T key, ThreadLocal *threadLocal) {
		int status;
		bool res;
		node *cur = NULL;
		START_OP(threadLocal);
		while (1) {
			assert(cur == NULL || cur->lock.isLocked());
			START_TEL(threadLocal);
			cur = teleport_cur(cur, key, threadLocal);
			assert(cur==NULL || cur->lock.isLocked());
			if (cur == NULL)
				return false;
			else if (cur->key == key) {
				cur->lock.release();
				return true;
			}
			assert(cur->lock.isLocked());
		}
	}
	//precondition: cur is locked or silently locked.
	int descendents(node *cur, T start, T end, T out[],
			ThreadLocal *threadLocal) {
		if (cur == NULL)
			return 0;
		int l = 0, r = 0;
		if (cur->key > start && cur->left != NULL) {
			cur->left->lock.silent_acq();
			l = descendents(cur->left, start, end, out, threadLocal);
		}
		if (cur->key >= start && cur->key <= end)
			out[l++] = cur->key;
		if (cur->key < end && cur->right != NULL) {
			cur->right->lock.silent_acq();
			r = descendents(cur->right, start, end, out + l, threadLocal);
		}
		return l + r;
	}
	//precondition: cur is either NULL or locked.
	//precondition: cur->key < kstart || cur->key > kend
	//precondition: reached==false;
	//postcondition: ret is either NULL or a descendant of cur (or head if cur is NULL) and is locked.
	//postcondition: reached==ret->key >= kstart && ret->key <= kend
	node* teleportRange(node *nstart, T kstart, T kend, bool &reached,
			ThreadLocal *threadLocal) {
		int status;
		assert(nstart==NULL || nstart->lock.isLocked());
		assert(nstart==NULL || nstart->key<kstart || nstart->key>kend);
		for (int i = 0; i < retries; ++i) {
			assert(nstart==NULL || nstart->lock.isLocked());
			if (Set::xbegin(threadLocal) == _XBEGIN_STARTED) {
				int limit = threadLocal->teleportLimit;
				node *cur = (nstart == NULL) ? head : nstart;
				while (cur != NULL && --limit) {
					unsigned curkey = cur->key;
					if (curkey < kstart) {
						cur = cur->right;
					} else if (curkey > kend) {
						cur = cur->left;
					} else {
						reached = true;
						break;
					}
					if (cur != NULL && cur->lock.isLocked())
						_xabort(XABORT_LOCKED);
				}
				if (cur != NULL)
					cur->lock.acquire_tsx_unlocked();
				_xend();
				assert(nstart==NULL || nstart->lock.isLocked());
				TEL_DIST(threadLocal, threadLocal->teleportLimit-limit);
				threadLocal->teleportLimit += 1;
				if (limit == 0)
					reached = (cur->key >= kstart && cur->key <= kend);
				assert(!(nstart!=NULL && nstart==cur));
				assert(nstart==NULL || nstart->lock.isLocked());
				if (nstart != NULL)
					nstart->lock.release();
				assert(cur==NULL || cur->lock.isLocked());
				assert(
						reached || cur==NULL || (cur->key<kstart || cur->key>kend));
				return cur;
			}
			assert(nstart==NULL || nstart->lock.isLocked());
			threadLocal->teleportLimit=(threadLocal->teleportLimit>MIN_TEL_LIMIT*2)?threadLocal->teleportLimit/2:MIN_TEL_LIMIT;
		}
		//fallback - non tsx teleport.
		FALLBACK(threadLocal);
		if (nstart == NULL) {
			nstart = head;
			nstart->lock.acquire();
			return nstart;
		}
		node *next;
		if (nstart->key < kstart)
			next = nstart->right;
		else
			next = nstart->left;
		if (next == NULL)
			return next;
		next->lock.acquire();
		nstart->lock.release();
		reached = (next->key >= kstart && next->key <= kend);
		assert(reached || next==NULL || (next->key<kstart || next->key>kend));
		return next;
	}

	int range(T start, T end, T out[], ThreadLocal *threadLocal) {
		node *cur = NULL;
		bool reached = false;
		volatile int ctr = 0;
		volatile TreeHT * volatile t = this;
		START_OP(threadLocal);
		do {
			START_TEL(threadLocal);
			cur = teleportRange(cur, start, end, reached, threadLocal);
			if (ctr >= 100)
				assert(0);
			assert(reached || cur==NULL || (cur->key<start || cur->key>end));
		} while (!reached && cur != NULL);
		if (cur == NULL)
			return 0;
		int res = descendents(cur, start, end, out, threadLocal);
		cur->lock.release();
		return res;
	}
	///-------------------------------------------------insert
	struct ret {
		node *prev, **pcur, *cur;
	};
	bool insertNode(ret location, unsigned key) {
		if (location.pcur == NULL)
			return false; //not sure what this case handles
		node *n = new node();
		n->left = n->right = NULL;
		n->key = key;
		n->obj = NULL;
		*location.pcur = n;
		location.prev->lock.release();
		return true;
	}
	virtual bool add(T key, ThreadLocal *threadLocal) {
		int status;
		ret res = { NULL, NULL, NULL };
		while (1) {
			int s = teleportModify(res, key, threadLocal);
			if (s == REACHED_VAL) {
				res.prev->lock.release();
				return false;
			} else if (s == REACHED_NULL)
				return insertNode(res, key);
		}
	}
	//////////////////////////////////////-----------------------------delete

	//assume: the cur & prev are locked by this thread
	//intput: @cur: the node to delete
	//@pcur: the address in the parent of cur when cur resides (&parent->left or &parent->right)
	void deleteNode(node *cur, node **pcur) {
		if (cur->right == NULL && cur->left == NULL) {
			*pcur = NULL;
		} else if (cur->right == NULL) {
			*pcur = cur->left;
		} else if (cur->left == NULL) {
			*pcur = cur->right;
		} else {
			node *rchild = cur->right;
			rchild->lock.silent_acq();
			if (rchild->left == NULL) {
				rchild->left = cur->left;
				*pcur = rchild;
			} else {
				node *rcur = rchild->left, *rprev = rchild, *rnext;
				rcur->lock.silent_acq();
				rnext = rcur->left;
				while (rnext != NULL) {
					rnext->lock.silent_acq();
					rprev = rcur;
					rcur = rnext;
					rnext = rnext->left;
				}
				rprev->left = rcur->right;	//disconnects cur
				//replace cur with rcur
				rcur->left = cur->left;
				rcur->right = cur->right;
				*pcur = rcur;
			}
		}
		delete cur;
	}
public:
	virtual bool remove(T key, ThreadLocal *threadLocal) {
		int status;
		ret res = { NULL, NULL, NULL };
		while (1) {
			int s = teleportModify(res, key, threadLocal);
			if (s == REACHED_VAL) {
				res.cur->lock.silent_acq();
				deleteNode(res.cur, res.pcur);
				res.prev->lock.release();
				return true;
			} else if (s == REACHED_NULL) {
				res.prev->lock.release();
				return false;
			}
		}
	}
private:
	enum {
		REACHED_NULL, REACHED_VAL, MIDWAY
	};
	//pre: pos.prev is locked
	//post: pos.prev is locked
	//ret: REACHED_NULL if pos.cur is null,
	//	   REACHED_VAL if pos.cur.key == k in this case pos.cur is locked (in addition to prev).
	//	   otherwise MIDWAY.
	inline int teleportModify(ret &pos, unsigned val,
			ThreadLocal* threadLocal) {
		int i = retries;
		int status, limit = threadLocal->teleportLimit;
		node *prev = pos.prev;
		do {
			if (START_TX() == _XBEGIN_STARTED) {
				if (pos.prev == NULL) {
					pos.prev = head;
					pos.cur = head->left;
					pos.pcur = &head->left;
				}
				if (pos.prev->lock.isLocked())
					_xabort(LOCK_HELD);
				while (pos.cur != NULL && --limit) {
					if (pos.cur->lock.isLocked())
						_xabort(LOCK_HELD);
					unsigned curkey = pos.cur->key;
					if (val == curkey) {
						break;
					}
					pos.prev = pos.cur;
					if (val < curkey)
						pos.pcur = &pos.cur->left;
					else
						pos.pcur = &pos.cur->right;
					pos.cur = *pos.pcur;
				}
				pos.prev->lock.acquire_tsx_unlocked();
				_xend();
				TEL_DIST(threadLocal, threadLocal->teleportLimit -limit);
				threadLocal->teleportLimit += 1;
				if (prev != NULL)
					prev->lock.release();
				if (pos.cur == NULL)
					return REACHED_NULL;
				if (limit == 0)
					return MIDWAY;
				return REACHED_VAL;
			}
			threadLocal->teleportLimit=(threadLocal->teleportLimit>MIN_TEL_LIMIT*2)?threadLocal->teleportLimit/2:MIN_TEL_LIMIT;
		} while (--i);
		FALLBACK(threadLocal);
		return no_tsx_teleport(pos, val, threadLocal);
	}
	int __attribute__((noinline)) no_tsx_teleport(ret &pos, unsigned val,
			ThreadLocal* threadLocal) {
		for (int i = 0; i < 10; i++) {
			int s = no_tsx_teleport_(pos, val, threadLocal);
			if (s != MIDWAY){
				if (pos.cur == NULL)
					return REACHED_NULL;
				return s;
			}
		}
		if(pos.cur==NULL)
			return REACHED_NULL;
		return MIDWAY;
	}
	inline int no_tsx_teleport_(ret &pos, unsigned val,
			ThreadLocal* threadLocal) {
		if (pos.prev == NULL) {
			pos.prev = head;
			pos.prev->lock.acquire();
			pos.pcur = &head->left;
			pos.cur = head->left;
			return MIDWAY;
		}
		if (pos.cur == NULL)
			return REACHED_NULL;
		pos.cur->lock.acquire();
		if (pos.cur->key == val) {
			pos.cur->lock.release();
			return REACHED_VAL;
		}
		pos.prev->lock.release();
		pos.prev = pos.cur;
		if (val < pos.cur->key)
			pos.pcur = &pos.prev->left;
		else
			pos.pcur = &pos.prev->right;
		pos.cur = *pos.pcur;

		return MIDWAY;
	}
public:
	node *build(int i, long first) { //build a full tree of depth i.
		if (i == 0)
			return NULL;
		node *n = new node();
		n->left = build(i - 1, first);
		n->right = build(i - 1, first + (1 << (i - 0)));
		n->key = first + (1 << (i - 0));
		n->obj = NULL;
		return n;
	}
	TreeHT() {
		head = new node();
		head->key = INT_MAX;
		head->left = NULL;
		head->right = NULL;
		head->obj = NULL;
	}
	void print(node *head, int l) {
		printf("start printing\n");
		try {
			int k = -1, kl = -1, kr = -1;
			k = (head) ? head->key : -1;
			if (head) {
				kl = (head->left) ? head->left->key : -1;
				kr = (head->right) ? head->right->key : -1;
			}
			printf(" %d\n%d %d\n", k, kl, kr);
		} catch (...) {
			printf("error while printing...\n");
		}
	}
	virtual std::string name() {
		return "tsx";
	}
}__attribute__((aligned((64))));

#endif /* TREE4_HPP_ */
