#include <cstring>
#include "ReorderBuffer.h"


ReorderBuffer::ReorderBuffer(size_t capacity):
	capacity(capacity),	front_(0), back_(capacity - 1), size_(0) {
	robs_.resize(capacity);
	for (int i = 0; i < capacity; ++i)
		robs_[i].Q = i + 1;
}

ReorderBuffer::~ReorderBuffer() {}

void ReorderBuffer::execute(const std::vector<ComnDataBus>& cdbs) {
	if (size_ == 0) return;
	int i = front_;
	do {
		// If this entry is waiting for data on CDB.
		if (robs_[i].busy) {
			for (auto iter = cdbs.begin(); iter != cdbs.end(); ++iter)
				if (iter->busy && i + 1 == iter->Q) {
					robs_[i].val = iter->val;
					robs_[i].busy = false;
					break;
				}
		}
	} while (i != back_);
}

void ReorderBuffer::tick() {
	if (size_ == 0) return;
	int i = front_;
	do { robs_[i].tick(); } while (i != back_);
}

int ReorderBuffer::push() {
	++size_;
	back_ = next(back_);
	return back_ + 1;
}

void ReorderBuffer::pop() {
	--size_;
	front_ = next(front_);
}

void ReorderBuffer::clear() {
	front_ = back_ = size_ = 0;
	robs_.clear();
	robs_.resize(capacity);
}

void ReorderBuffer::withdraw() { 
	--size_;
	back_ = (back_ + capacity - 1) % capacity;
}

void ReorderBuffer::Entry::tick() {
	busy.tick();
	dest_reg.tick();
	val.tick();
	addr.tick();
	cmd.tick();
}
