#pragma once

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

		void tick();
	};
private:
	Entry* robs_;

	int front_, back_, size_;

	int next(int i) { return (i + 1ll) % capacity; }

public:
	const size_t capacity;

	ReorderBuffer(size_t capacity);

	~ReorderBuffer();

	bool empty() const { return size_ == 0; }

	bool full() const { return size_ == capacity; }

	void tick(const std::vector<ComnDataBus>& cdbs);

	int push();

	void pop();

	void clear();

	void withdraw();

	Entry& front() { return robs_[front_]; }

	Entry& back() { return robs_[back_]; }

	Entry& operator[] (int index) { return robs_[index - 1]; }
};
