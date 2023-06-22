#pragma once
#ifndef RV_TOMASULO_CORE_H
#define RV_TOMASULO_CORE_H

#include <queue>
#include <vector>
#include "RvMemory.h"
#include "RvALU.h"
#include "ReorderBuffer.h"

struct MemUnit {
	std::queue<uint32> load_que_addr;

	std::queue<uint32> load_que_cmd;

	std::queue<int8> load_que_Q;

	std::queue<size_t> load_que_time;

	RvMemory* pmem = nullptr;

	void tick(ComnDataBus* cdb);
};

class TomasuloCPUCore {
private:
	std::vector<ComnDataBus> cdbs_;

	std::vector<ALU> alus_;

	Register<uint32> PC;

	Register<int32> reg[32];

	Register<int8> reg_file[32];

	MemUnit mu_;

	ReorderBuffer rob_;

	const size_t cdbn_, alun_, robn_;

	ComnDataBus* get_idle_cdb_();

	void issue_();

	void execute_();

	void write_result_();

	void commit_();

	bool try_get_data_(int8 r, int32& val);

	bool try_get_alu_(int& i);

public:
	TomasuloCPUCore(RvMemory* mem, size_t cdbn, size_t alun, size_t robn);

	~TomasuloCPUCore();

	void tick();
};

#endif // !RV_TOMASULO_CORE_H