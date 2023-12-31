#pragma once
#ifndef REORDERBUFFER_H
#define REORDERBUFFER_H

#include "Types.h"
#include "RvComnDataBus.h"
#include "RvRegister.hpp"
#include <vector>

class ReorderBuffer
{
public:
	struct Entry {
		Register<bool> busy;

		Register<int8> dest_reg;

		Register<int32> val;

		Register<uint32> addr, cmd;

		int8 Q;

		void tick();
	};
private:
	std::vector<Entry> robs_;

	Register<int> front_, back_, size_;

	int next(int i) { return (i + 1ll) % capacity; }

public:
	struct iterator {
	private:
		ReorderBuffer* rob_;

		int index_;

	public:
		iterator(ReorderBuffer* rob, int index);

		iterator(const iterator& other);

		iterator& operator++();

		iterator operator++(int);

		Entry* operator->();

		bool operator!= (const iterator& other);
	};

	iterator begin();

	iterator end();

	friend iterator;

	const size_t capacity;

	ReorderBuffer(size_t capacity);

	~ReorderBuffer();

	bool empty() const { return size_ == 0; }

	bool full() const { return size_ == capacity; }

	void write_result_(const std::vector<ComnDataBus>& cdbs);

	void tick();

	Entry& push();

	void pop();

	void clear();

	void withdraw();

	Entry& front() { return robs_[front_]; }

	Entry& back() { return robs_[back_]; }

	Entry& operator[] (int index) { return robs_[index - 1]; }
};

#endif //!REORDERBUFFER_H