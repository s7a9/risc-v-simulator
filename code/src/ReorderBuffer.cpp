#include <cstring>
#include "ReorderBuffer.h"

using iterator = ReorderBuffer::iterator;

iterator ReorderBuffer::begin() {
	return iterator(this, front_);
}

iterator ReorderBuffer::end() {
	return iterator(this, back_);
}

ReorderBuffer::ReorderBuffer(size_t capacity):
	capacity(capacity),	front_(0), back_(capacity - 1), size_(0) {
	robs_.resize(capacity);
	for (int i = 0; i < capacity; ++i)
		robs_[i].Q = i + 1;
}

ReorderBuffer::~ReorderBuffer() {}

void ReorderBuffer::execute(const std::vector<ComnDataBus>& cdbs) {
	if (size_ == 0) return;
	auto iter = begin();
	do {
		if (iter->busy) for (auto& cdb : cdbs) {
			if (cdb.busy && iter->Q == (cdb.Q & 31)) {
				if (cdb.Q & 32) {
					if ((iter->cmd & 127) == Instr_L) {
						iter->addr = cdb.val;
						iter->val = 1;
					}
				}
				else {
					iter->val = cdb.val;
					iter->busy = false;
				}
				break;
			}
		}
	} while (iter++ != end());
}

void ReorderBuffer::tick() {
	front_.tick();
	back_.tick();
	size_.tick();
	if (size_ == 0) return;
	auto iter = begin();
	do {
		iter->tick();
	} while (iter++ != end());
}

ReorderBuffer::Entry& ReorderBuffer::push() {
	size_ = size_.nxt_data + 1;
	back_ = next(back_.nxt_data);
	return robs_[back_.nxt_data];
}

void ReorderBuffer::pop() {
	size_ = size_.nxt_data - 1;
	front_ = next(front_.nxt_data);
}

void ReorderBuffer::clear() {
	front_ = size_ = 0;
	back_ = capacity - 1;
	robs_.clear();
	robs_.resize(capacity);
	for (int i = 0; i < capacity; ++i)
		robs_[i].Q = i + 1;
}

void ReorderBuffer::withdraw() { 
	size_ = size_.nxt_data - 1;
	back_ = (back_.nxt_data + capacity - 1) % capacity;
}

void ReorderBuffer::Entry::tick() {
	busy.tick();
	dest_reg.tick();
	val.tick();
	addr.tick();
	cmd.tick();
}

iterator::iterator(ReorderBuffer* rob, int index) :
	rob_(rob), index_(index) {}

iterator::iterator(const iterator& other) :
	rob_{ other.rob_ }, index_(other.index_) {}

iterator& iterator::operator++() {
	index_ = rob_->next(index_);
	return *this;
}

iterator iterator::operator++(int) {
	iterator iter = *this;
	++(*this);
	return iter;
}

ReorderBuffer::Entry* iterator::operator->() {
	return rob_->robs_.data() + index_;
}

bool ReorderBuffer::iterator::operator!=(const iterator& other) {
	return index_ != other.index_;
}

