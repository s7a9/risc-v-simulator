#pragma once
#ifndef RVCOMNDATABUS_H
#define RVCOMNDATABUS_H

#include "Types.h"

struct ComnDataBus {
	bool busy;

	int8 Q;

	int32 val;
};

#endif //!RVCOMNDATABUS_H