/**
 * hand-over-hand list
 * hazard pointers
 */
#include "ListHZ.h"

ListHZ::ListHZ():
LSentinel(LONG_MIN),
RSentinel(LONG_MAX),
manager() {
	LSentinel.next = (List::Node*) &RSentinel;
};

bool ListHZ::add(T v, ThreadLocal* threadLocal) {
	List::Node* pred = &LSentinel;
	pred->lock();
	List::Node* curr = pred->next;
	curr->lock();
	while (curr->value < v) {
		pred->unlock();
		pred = curr;
		curr = curr->next;
		curr->lock();
	}
	if (curr->value == v) {
		pred->unlock();
		curr->unlock();
		return false;
	}
	List::Node* node = manager.alloc(v, threadLocal);
	node->next = curr;
	pred->next = node;
	pred->unlock();
	curr->unlock();
	return true;
};


bool ListHZ::remove(T v, ThreadLocal* threadLocal) {
	List::Node* pred = &LSentinel;
	pred->lock();
	List::Node* curr = pred->next;
	curr->lock();
	while (curr->value < v) {
		pred->unlock();
		pred = curr;
		curr = curr->next;
		curr->lock();
	}
	if (curr->value == v) {
		pred->next = curr->next;
		pred->unlock();
		manager.retire(curr, threadLocal);
		curr->unlock();
		return true;
	} else {
		pred->unlock();
		curr->unlock();
		return false;
	}
};

bool ListHZ::contains(T v, ThreadLocal* threadLocal) {
	List::Node* pred = &LSentinel;
	pred->lock();
	List::Node* curr = LSentinel.next;
	curr->lock();
	while (curr->value < v) {
		pred->unlock();
		pred = curr;
		curr = curr->next;
		curr->lock();
	}
	T value = curr->value;
	pred->unlock();
	curr->unlock();
	return value == v;
};


int ListHZ::size() {
	int result = 0;
	Node* node = LSentinel.next;
	while (node != &RSentinel) {
		result++;
		node = node->next;
	}
	return result;
}
bool ListHZ::sanity() {
	Node* node = LSentinel.next;
	bool ok = true;
	char buf[128];
	while (node != &RSentinel) {
		if (node->isLocked()) {
			ok = false;
		}
		node = node->next;
	}
	return ok;
}

void ListHZ::display() {		// for validation, not thread-safe
	Node* node = LSentinel.next;
	char buf[128];
	while (node != &RSentinel) {
		node = node->next;
	}
}
