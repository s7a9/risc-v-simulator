#pragma once
#ifndef RV_PREDICTOR_H
#define RV_PREDICTOR_H

#include "Types.h"

class Predictor {
public:
	int stat[2][2];

	Predictor();

	virtual bool predict(uint32 addr) = 0;

	virtual void revise(uint32 addr, bool pred, bool real);
};

class AlwaysPredictor : public Predictor {
public:
	bool predict(uint32 addr) { return true; }
};

class AlwaysNotPredictor : public Predictor {
public:
	bool predict(uint32 addr) { return false; }
};

class TwoBitSaturatePredictor : public Predictor {
private:
	const size_t size_;

	int8* record_;

public:
	TwoBitSaturatePredictor(size_t size);

	~TwoBitSaturatePredictor();

	bool predict(uint32 addr);

	void revise(uint32 addr, bool pred, bool real);
};

#endif //!RV_PREDICTOR_H