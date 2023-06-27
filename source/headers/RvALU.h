#pragma once
#ifndef RVALU_H
#define RVALU_H

#include "RvComnDataBus.h"
#include "RvRegister.hpp"
#include <vector>

enum RS_State {
	RS_Idle, RS_Waiting, RS_Ready
};

struct ResrvStation {
	Register<int8> Op, Q1, Q2, Dst, state;

	Register<int32> V1, V2;

	Register<uint32> cmd;

	void write_result_(const std::vector<ComnDataBus>& cdbs);

	void tick();
};

struct ALU {
	static void execute(ComnDataBus* cdb, ResrvStation& rs);
};

#endif //!RVALU_H