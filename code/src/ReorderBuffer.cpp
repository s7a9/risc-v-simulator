#include <cstring>
#include "ReorderBuffer.h"


ReorderBuffer::ReorderBuffer(size_t capacity):
	capacity(capacity), robs_(new Entry[capacity]),
	front_(0), back_(capacity - 1), size_(0) {}

ReorderBuffer::~ReorderBuffer() {
	delete[] robs_;
}

void ReorderBuffer::tick(const std::vector<ComnDataBus>& cdbs) {
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
		robs_[i].tick();
	} while (i != back_);
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
	memset(robs_, 0, sizeof(Entry) * capacity);
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
