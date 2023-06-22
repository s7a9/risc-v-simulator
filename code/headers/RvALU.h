#pragma once

#include "RvComnDataBus.h"
#include "RvRegister.hpp"

enum RS_State {
	RS_Empty, RS_Waiting, RS_Ready, RS_Committed
};

struct ResrvStation {
	Register<int8> Op, Q1, Q2, Dst, state;

	Register<int32> V1, V2;

	Register<uint32> cmd;

	void execute(ComnDataBus* cdb, size_t cdbn);

	void tick();
};

struct ALU {
	ResrvStation rs;

	void execute(ComnDataBus* cdb);
};