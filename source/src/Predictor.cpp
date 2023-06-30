#include <cstring>
#include "Predictor.h"

Predictor::Predictor() {
	memset(stat, 0, sizeof(stat));
}

void two_bit_revise(int8& r, bool real) {
	if (real && r != 3) ++r;
	if (!real && r) --r;
}

void Predictor::revise(uint32 addr, bool pred, bool real) {
	++stat[real][pred];
}

TwoBitSaturatePredictor::TwoBitSaturatePredictor(size_t size) :
	size_(1ull << size), record_(new int8[size_]) {
	memset(record_, 0, size_);
}

TwoBitSaturatePredictor::~TwoBitSaturatePredictor() {
	delete[] record_;
}

bool TwoBitSaturatePredictor::predict(uint32 addr) {
	addr = (addr >> 2) & (size_ - 1);
	return record_[addr] & 2;
}

void TwoBitSaturatePredictor::revise(uint32 addr, 
	bool pred, bool real) {
	int8& r = record_[(addr >> 2) & (size_ - 1)];
	two_bit_revise(r, real);
	++stat[real][pred];
}

LocalPredictor::LocalPredictor(size_t size): size_(1ull << size) {
	record_ = new int8[size_];
	history_ = new int[size_];
	memset(record_, 0, sizeof(int8) * size_);
	memset(history_, 0, sizeof(int) * size_);
}

LocalPredictor::~LocalPredictor() {
	delete[] record_;
	delete[] history_;
}

bool LocalPredictor::predict(uint32 addr) {
	return record_[history_[(addr >> 2) & (size_ - 1)] & (size_ - 1)] & 2;
}

void LocalPredictor::revise(uint32 addr, bool pred, bool real) {
	int& h = history_[(addr >> 2) & (size_ - 1)];
	int8& r = record_[h & (size_ - 1)];
	h = (h << 1) + real;
	two_bit_revise(r, real);
	++stat[real][pred];
}

GSharePredictor::GSharePredictor(size_t size) : size_(1ull << size) {
	record_ = new int8[size_];
	history_ = new int[size_];
	memset(record_, 0, size_);
	memset(history_, 0, sizeof(int) * size_);
}

GSharePredictor::~GSharePredictor() {
	delete[] record_;
	delete[] history_;
}

bool GSharePredictor::predict(uint32 addr) {
	addr = (addr >> 2) & (size_ - 1);
	return record_[addr ^ (history_[addr] & (size_ - 1))] & 2;
}

void GSharePredictor::revise(uint32 addr, bool pred, bool real) {
	addr = (addr >> 2) & (size_ - 1);
	int8& r = record_[addr ^ (history_[addr] & (size_ - 1))];
	two_bit_revise(r, real);
	++stat[real][pred];
	history_[addr] = (history_[addr] << 1) + real;
}

TournamentPredictor::TournamentPredictor(size_t size) : size_(1ull << size) {
	local_  = new int8[size_];
	global_ = new int8[size_];
	select_ = new int8[size_];
	history_ = new int[size_];
	memset(local_,  0, size_);
	memset(global_, 0, size_);
	memset(select_, 0, size_);
	memset(history_, 0, sizeof(int) * size_);
}

TournamentPredictor::~TournamentPredictor() {
	delete[] local_;
	delete[] global_;
	delete[] select_;
	delete[] history_;
}

bool TournamentPredictor::predict(uint32 addr) {
	addr = (addr >> 2) & (size_ - 1);
	if (select_[addr] & 2) return global_[addr] & 2;
	return local_[history_[addr] & (size_ - 1)] & 2;
}

void TournamentPredictor::revise(uint32 addr, bool pred, bool real) {
	addr = (addr >> 2) & (size_ - 1);
	int8& g = global_[addr];
	int8& s = select_[addr];
	int8& l = local_[history_[addr] & (size_ - 1)];
	history_[addr] = (history_[addr] << 1) + real;
	if (s & 2) {
		two_bit_revise(s, ((g & 2) != 0) == real);
		two_bit_revise(g, real);
	}
	else {
		two_bit_revise(s, ((l & 2) == 0) == real);
		two_bit_revise(l, real);
	}
	++stat[real][pred];
}

