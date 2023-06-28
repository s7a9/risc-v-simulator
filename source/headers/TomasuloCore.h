#pragma once
#ifndef RV_TOMASULO_CORE_H
#define RV_TOMASULO_CORE_H

#include <queue>
#include <vector>
#include "RvMemory.h"
#include "Predictor.h"
#include "RvALU.h"
#include "ReorderBuffer.h"

struct MemUnit {
	static const size_t MEM_LOAD_TIME = 3;

	struct load_instr {
		int8 Q;

		uint32 addr, cmd;

		size_t time;
	};

	std::queue<load_instr> load_queue;

	RvMemory* pmem = nullptr;

	void execute(ComnDataBus* cdb, size_t time);
};

class TomasuloCPUCore {
private:
	std::vector<ComnDataBus> cdbs_;

	std::vector<ResrvStation> alus_;

	Register<uint32> PC;

	Register<int32> reg[32];

	Register<int8> reg_file[32];

	MemUnit mu_;

	ReorderBuffer rob_;

	size_t cpu_time_;

	const size_t cdbn_, alun_, robn_;

	Predictor* predictor_;

	bool mispredicted_;

	ComnDataBus* get_idle_cdb_();

	bool issue_();

	void execute_();

	void write_result_();

	void commit_();

	bool try_get_data_(int8 r, int32& val);

	ResrvStation* try_get_alu_();

	bool load_rs_data_(ResrvStation& rs, int8 rs1, int8 rs2);

public:
	bool end_simulate = false;

	int32 simulate_result = 0;

	size_t cpu_time() const { return cpu_time_; }

	TomasuloCPUCore(RvMemory* mem, Predictor* predictor,
		size_t cdbn, size_t alun, size_t robn);

	~TomasuloCPUCore();

	void tick();
};

#endif // !RV_TOMASULO_CORE_H