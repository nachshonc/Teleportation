/*
 * treeH.hpp
 *
 *  Created on: Jan 18, 2015
 *      Author: nachshonc
 */

#ifndef TREEH_HPP_
#define TREE3H_HPP_
#include "Set.h"
#include "common/locks.hpp"
#include <string>
#include <iostream>
#include <limits.h>
#include <assert.h>
using namespace std;

class TreeH: public Set{
public:
	struct node{
		unsigned key;
		node *left, *right;
		void *obj;
		tatas_lock lock;
	};
	node *head;
	bool contains(T key, ThreadLocal *threadLocal){
		node *cur=head;
		cur->lock.acquire();
		while(cur!=NULL){
			unsigned curkey = cur->key;
			if(key==curkey) {
				cur->lock.release();
				return true;
			}
			node *prev=cur;
			if(key<curkey)
				cur=cur->left;
			else
				cur = cur->right;
			if(cur!=NULL) cur->lock.acquire();
			prev->lock.release();
		}
		return false;
	}
private:
	int descendents(node *cur, T start, T end,
			T out[], ThreadLocal *threadLocal){
		if(cur==NULL) return 0;
		int l=0, r=0;
		if(cur->key>start && cur->left!=NULL){
			cur->left->lock.silent_acq();//an ancestor is locked. No other thread can lock this node. Just wait previous threads release it.
			l=descendents(cur->left, start, end, out, threadLocal);
		}
		if(cur->key>=start && cur->key<=end)
			out[l++]=cur->key;
		if(cur->key<end && cur->right!=NULL){
			cur->right->lock.silent_acq();
			r=descendents(cur->right, start, end, out+l, threadLocal);
		}
		return l+r;
	}
public:
	int range(T start, T end, T out[], ThreadLocal *threadLocal){
		node *cur=head, *prev;
		cur->lock.acquire();
		while(cur!=NULL){
			unsigned curkey = cur->key;
			prev=cur;
			if(curkey<start){
				cur=cur->right;
			}
			else if(curkey>end){
				cur=cur->left;
			}
			else{
				int res= descendents(cur, start, end, out, threadLocal);
				cur->lock.release();
				return res;
			}
			if(cur!=NULL) cur->lock.acquire();
			prev->lock.release();
		}
		return 0;
	}
	virtual bool add(T key, ThreadLocal *threadLocal){
		node *cur=head;
		cur->lock.acquire();
		while(cur!=NULL){
			unsigned curkey = cur->key;
			if(key==curkey) {
				cur->lock.release();
				return false; //key already exists.
			}
			node *prev=cur;
			node **pcur;
			if(key<curkey)
				pcur=&cur->left;
			else{
				pcur=&cur->right;
			}
			cur=*pcur;
			if(cur!=NULL) {
				cur->lock.acquire();
			}
			else{
				node *n=new node();
				n->left=n->right=NULL;
				n->obj=NULL;
				n->key=key;
				*pcur=n;
				prev->lock.release();
				return true;
			}
			prev->lock.release();
		}
		return false;
	}
private:
	//assume: the cur & prev are lock by this thread
	//intput: @cur: the node to delete
	//@pcur: the address in the parent of cur when cur resides (&parent->left or &parent->right)
	void deleteNode(node *cur, node **pcur){
		if(cur->right==NULL && cur->left==NULL){
			*pcur=NULL;
			delete cur;
		}
		else if(cur->right==NULL){
			*pcur=cur->left;
			delete cur;
		}
		else if(cur->left==NULL){
			*pcur=cur->right;
			delete cur;
		}
		else{
			node *rchild=cur->right;
			rchild->lock.silent_acq();
			if(rchild->left==NULL){
				rchild->left=cur->left;
				*pcur=rchild;
				delete cur;
			}
			else{
				node *rcur=rchild->left, *rprev=rchild, *rnext;
				rcur->lock.silent_acq();
				rnext = rcur->left;
				while(rnext!=NULL){
					rnext->lock.silent_acq();
					rprev=rcur;
					rcur=rnext;
					rnext=rnext->left;
				}
				rprev->left=rcur->right;//disconnects cur
				//replace cur with rcur
				rcur->left=cur->left;
				rcur->right=cur->right;
				*pcur = rcur;
				delete cur;
			}
		}
	}
public:
	virtual bool remove(T key, ThreadLocal *threadLocal){
		node *cur=head, *prev=NULL;
		cur->lock.acquire();
		assert(cur==head);
		while(cur!=NULL){
			unsigned curkey = cur->key;
			if(key==curkey) {
				cur->lock.release();
				return true;
			}
			prev=cur;
			node **pcur;
			if(key<curkey)
				pcur=&cur->left;
			else
				pcur=&cur->right;
			cur=*pcur;
			if(cur!=NULL){
				cur->lock.acquire();
				if(cur->key==key){
					deleteNode(cur, pcur);
					prev->lock.release();
					//note that cur does not exist anymore...
					return true;
				}
			}
			prev->lock.release();
		}
		return false;
	}

	node *build(int i, long first){//build a full tree of level i
		if(i==0) return NULL;
		node *n = new node();
		n->left = build(i-1, first);
		n->right = build(i-1, first + (1<<(i-0)));
		n->key = first + (1<<(i-0));
		n->obj=NULL;
		return n;
	}
	TreeH(){
		head=new node();
		head->key=INT_MAX;
		head->left=NULL; //build(16, 0);
		head->right=NULL;
		head->obj=NULL;
	}
	TreeH(int depth){
		head=new node();
		head->key=INT_MAX;
		head->left=build(depth, 0);
		head->right=NULL;
		head->obj=NULL;
	}
	void print(node *head, int l){
		printf("start printing\n");
		try{
			int k=-1, kl=-1, kr=-1;
			k=(head)?head->key:-1;
			if(head){
				kl=(head->left)?head->left->key:-1;
				kr=(head->right)?head->right->key:-1;
			}
			printf(" %d\n%d %d\n", k, kl, kr);
		}catch(...){
			printf("error while printing...\n");
		}
	}
	virtual std::string name(){return "HOH"; }
};

#endif /* TREE3_HPP_ */
