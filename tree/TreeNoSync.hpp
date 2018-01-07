/*
 * tree1.hpp
 *
 *  Created on: Jan 18, 2015
 *      Author: nachshonc
 */

#ifndef TREENO_SYNC_HPP_
#define TREENO_SYNC_HPP_
#include "Set.h"
#include <string>
#include <limits.h>
#include <iostream>
using namespace std;

class TreeNoSync: public Set{
public:
	struct node{
		unsigned key;
		node *left, *right;
		void *obj;
	};
	node *head;
	bool contains(T key, ThreadLocal *thread){
		node *cur=head;
		while(cur!=NULL){
			unsigned curkey = cur->key;
			if(key==curkey) return true;
			else if(key<curkey)
				cur=cur->left;
			else
				cur = cur->right;
		}
		return false;
	}
	virtual bool add(T o, ThreadLocal* threadLocal){
		return contains(o, threadLocal);
	}
	virtual bool remove(T o, ThreadLocal* threadLocal){
		return contains(o, threadLocal);
	}
	int descendents(node *cur, T start, T end,
			T out[], ThreadLocal *threadLocal){
		if(cur==NULL) return 0;
		int l=0, r=0;
		if(cur->key>start)
			l=descendents(cur->left, start, end, out, threadLocal);
		if(cur->key>=start && cur->key<=end)
			out[l++]=cur->key;
		if(cur->key<end)
			r=descendents(cur->right, start, end, out+l, threadLocal);
		return l+r;
	}
	virtual int range(T start, T end, T out[], ThreadLocal *threadLocal){
		return descendents(head, start, end, out, threadLocal);
	}
	node *build(int i, long first){
		if(i==0) return NULL;
		node *n = new node();
		n->left = build(i-1, first);
		n->right = build(i-1, first + (1<<(i-0)));
		n->key = first + (1<<(i-0));
		n->obj=NULL;
		return n;
	}
	TreeNoSync(){
		head=build(1, INT_MAX);
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
	virtual std::string name(){return "Opt"; }
};

#endif /* TREE1_HPP_ */
