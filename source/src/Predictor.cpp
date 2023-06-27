#include <cstring>
#include "Predictor.h"

Predictor::Predictor() {
	memset(stat, 0, sizeof(stat));
}

void Predictor::revise(uint32 addr, bool pred, bool real) {
	++stat[real][pred];
}

TwoBitSaturatePredictor::TwoBitSaturatePredictor(size_t size) :
	size_(size), record_(new int8[size]) {
	memset(record_, 0, size_);
}

TwoBitSaturatePredictor::~TwoBitSaturatePredictor() {
	delete[] record_;
}

bool TwoBitSaturatePredictor::predict(uint32 addr) {
	addr = (addr >> 2) % size_;
	return record_[addr] & 2;
}

void TwoBitSaturatePredictor::revise(uint32 addr, 
	bool pred, bool real) {
	int8& r = record_[(addr >> 2) % size_];
	if (real && r != 3) ++r;
	if (!real && r) --r;
	++stat[real][pred];
}

