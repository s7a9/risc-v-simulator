#include "TomasuloCore.h"

#define parse_op(cmd)		(int8)(cmd & 127)
#define parse_rd(cmd)		(int8)((cmd >> 7 ) & 31)
#define parse_rs1(cmd)		(int8)((cmd >> 15) & 31)
#define parse_rs2(cmd)		(int8)((cmd >> 20) & 31)
#define parse_func3(cmd)	(int8)((cmd >> 12) & 7)

inline int32 extend_sign(uint32 x, int8 bitc) {
	if (bitc && (x >> (bitc - 1))) {
		x |= 0xFFFFFFFF << bitc;
	}
	return x;
}

int32 parse_imm(uint32 cmd, char type) {
	switch (type) {
	case 'I': return extend_sign(
			cmd >> 20, 12);
	case 'S': return extend_sign(
			((cmd & 0xFE000000) >> 20) |
			((cmd & 0xF80) >> 7), 12);
	case 'B': return extend_sign(
			((cmd & 0x80000000) >> 19) |
			((cmd & 0x7E000000) >> 20) |
			((cmd & 0x00000F00) >> 7)  |
			((cmd & 0x00000080) << 4), 12);
	case 'U': return cmd & 0xFFFFF000;
	case 'J': return extend_sign(
			((cmd & 0x80000000) >> 11) |
			((cmd & 0x7FE00000) >> 20) |
			((cmd & 0x00100000) >> 9)  |
			((cmd & 0x000FF000)), 20);
	}
	return 0;
}

ComnDataBus* TomasuloCPUCore::get_idle_cdb_() {
	for (int i = 0; i < cdbn_; ++i) {
		if (!cdbs_[i].busy) return &cdbs_[i];
	}
	return nullptr;
}

TomasuloCPUCore::~TomasuloCPUCore() {}

void TomasuloCPUCore::tick() {
	// Only requirement for order:
	// execute must be before write result

	// Stage 1: Issue
	//   Issue an instruction into RS and write it into ROB (if available)
	issue_();
	// Stage 2: Execute
	//   ALUs and Mem Unit write output onto CDB
	execute_();
	// Stage 3: Write Result
	//   Use data on CDB to update ROB and RS
	write_result_();
	// Stage 4: Commit
	//   If the head of ROB is not busy, commit the instruction to registers or memory
	//   Note #1: Solve branch misprediction by emptying up ROB.
	//   Note #2: Immediately when load instruction receive address from cdb, it will send load command to MU. 
	//            (if there are no store commands before it)
	commit_();
	if (mispredicted_) {
		mispredicted_ = false;
		rob_.clear();
		while (!mu_.load_queue.empty())
			mu_.load_queue.pop();
		for (auto& iter : alus_) iter.state = RS_Idle;
		for (int i = 0; i < 32; ++i) reg_file[i] = 0;
	}
	for (auto& iter : alus_) iter.tick();
	reg[0] = 0; reg_file[0] = 0;
	for (int i = 0; i < 32; ++i) {
		reg[i].tick();
		reg_file[i].tick();
	}
	PC.tick();
	rob_.tick();
	++cpu_time_;
}

TomasuloCPUCore::TomasuloCPUCore(RvMemory* pmem, Predictor* predictor,
	size_t cdbn, size_t alun, size_t robn):
	rob_(robn), cdbn_(cdbn), alun_(alun), robn_(robn) {
	mu_.pmem = pmem;
	predictor_ = predictor;
	cpu_time_ = 0;
	mispredicted_ = false;
	cdbs_.resize(cdbn);
	alus_.resize(alun);
}

#define WITHDRAW_AND_RETURN {PC = PC;rob_.withdraw();return false;}

bool TomasuloCPUCore::issue_() {
	// Reoerder Buffer is Full, cannot issue instruction
	if (rob_.full()) return false;
	// The priority of misprediction handling is higher than issue
	if (mispredicted_) return false;
	ReorderBuffer::Entry& rob = rob_.push();
	ResrvStation* rs = nullptr;
	int32 tmp_val = 0;
	uint32 cmd;
	mu_.pmem->read(PC, cmd);
	if (cmd == 0x0ff00513) {
		simulate_result = reg[10];
		end_simulate = true;
		return false;
	}
	int8 opcode = cmd & 127;
	int8 rd = parse_rd(cmd);
	PC = PC + 4;
	rob.busy = true;
	rob.cmd = cmd;
	rob.dest_reg = rd;
	rob.addr = PC;
	switch (opcode)	{
	case Instr_LUI: 
		rob.busy = false;
		reg_file[rd] = rob.Q;
		rob.val = parse_imm(cmd, 'U');
		break;
	case Instr_AUIPC:
		if (!(rs = try_get_alu_())) {
			WITHDRAW_AND_RETURN;
		}
		rob.busy = true;
		reg_file[rd] = rob.Q;
		rs->cmd = cmd; rs->Dst = rob.Q;
		rs->Op = rs->Q1 = rs->Q2 = 0;
		rs->state = RS_Ready;
		rs->V1 = PC;
		rs->V2 = parse_imm(cmd, 'U');
		break;
	case Instr_JAL:
		rob.busy = false;
		reg_file[rd] = rob.Q;
		rob.val = PC + 4;
		PC = PC + parse_imm(cmd, 'J');
		break;
	case Instr_JALR:
		if (try_get_data_(parse_rs1(cmd), tmp_val)) {
			rob.busy = false;
			reg_file[rd] = rob.Q;
			rob.val = PC + 4;
			PC = (tmp_val + (parse_imm(cmd, 'I') << 1)) & 0xFFFFFFFE;
		}
		else { 
			// Failed to get data from registers or ROB
			// issue MUST be STALLED and wait for rs1 to be calculated
			// OR cpu would get wrong PC
			WITHDRAW_AND_RETURN;
		}
		break;
	case Instr_Branch:
		if (!(rs = try_get_alu_())) {
			WITHDRAW_AND_RETURN;
		}
		rob.busy = true;
		rs->cmd = cmd; rs->Dst = rob.Q;
		rs->Op = parse_func3(cmd);
		rs->state = load_rs_data_(*rs, parse_rs1(cmd), parse_rs2(cmd)) ?
			RS_Ready : RS_Waiting;
		if (predictor_->predict(PC)) {
			PC = PC + parse_imm(cmd, 'B');
			rob.dest_reg = 1;
		}
		else rob.dest_reg = 0;
		break;
	case Instr_L:
		if (!(rs = try_get_alu_())) {
			WITHDRAW_AND_RETURN;
		}
		rob.busy = true;
		rob.val = 0;
		reg_file[rd] = rob.Q;
		rs->cmd = cmd;
		rs->Dst = rob.Q | 32;
		rs->Op = 0;
		rs->state = load_rs_data_(*rs, parse_rs1(cmd), 0) ?
			RS_Ready : RS_Waiting;
		rs->V2 = parse_imm(cmd, 'I');
		break;
	case Instr_S:
		if (!(rs = try_get_alu_())) {
			WITHDRAW_AND_RETURN;
		}
		rob.dest_reg = 0;
		rob.busy = true;
		rs->cmd = cmd; rs->Dst = rob.Q;
		rs->Op = 0;
		rs->state = load_rs_data_(*rs, parse_rs1(cmd), 0) ?
			RS_Ready : RS_Waiting;
		rs->V2 = parse_imm(cmd, 'S');
		break;
	case Instr_ALUI:
		if (!(rs = try_get_alu_())) {
			WITHDRAW_AND_RETURN;
		}
		rob.busy = true;
		reg_file[rd] = rob.Q;
		rs->cmd = cmd; rs->Dst = rob.Q;
		rs->Op = parse_func3(cmd);
		rs->state = load_rs_data_(*rs, parse_rs1(cmd), 0) ?
			RS_Ready : RS_Waiting;
		rs->V2 = parse_imm(cmd, 'I');
		break;
	case Instr_ALU:
		if (!(rs = try_get_alu_())) {
			WITHDRAW_AND_RETURN;
		}
		rob.busy = true;
		reg_file[rd] = rob.Q;
		rs->cmd = cmd; rs->Dst = rob.Q;
		rs->Op = parse_func3(cmd);
		rs->state = load_rs_data_(*rs, parse_rs1(cmd), parse_rs2(cmd)) ?
			RS_Ready : RS_Waiting;
		break;
	}
	return true;
}

void TomasuloCPUCore::execute_() {
	for (auto& iter : cdbs_) iter.busy = false; 
	mu_.execute(get_idle_cdb_(), cpu_time_);
	for (auto& iter : alus_) {
		ComnDataBus* cdb = get_idle_cdb_();
		if (!cdb) break;
		ALU::execute(cdb, iter);
	}
}

void TomasuloCPUCore::write_result_() {
	for (auto& iter : alus_) {
		ResrvStation& rs = iter;
		rs.write_result_(cdbs_);
		// Allow reservestation to read data from ROB
		if (rs.state == RS_Waiting) {
			if (rs.Q1 && !rob_[rs.Q1].busy) {
				rs.V1 = rob_[rs.Q1].val;
				rs.Q1 = 0;
			}
			if (rs.Q2 && !rob_[rs.Q2].busy) {
				rs.V2 = rob_[rs.Q2].val;
				rs.Q2 = 0;
			}
			if (!rs.Q1.nxt_data && !rs.Q2.nxt_data)
				rs.state = RS_Ready;
		}
	}
	rob_.write_result_(cdbs_);
}

void TomasuloCPUCore::commit_() {
	if (rob_.empty()) return;
	ReorderBuffer::Entry& rob = rob_.front();
	int32 val = 0;
	// If any load instruction is ready with memory address
	// Push the command into MU
	// Should not push load instruction after store (RAW hazard)
	auto iter = rob_.begin();
	do {
		// Load may have been committed, which would be loaded twice and crash
		if (!iter->busy) continue;   
		int8 opcode = iter->cmd & 127;
		if (opcode == Instr_S) break;
		if (opcode == Instr_L && iter->val == 1) {
			iter->val = 0;
			mu_.load_queue.push(MemUnit::load_instr{
				iter->Q, iter->addr, iter->cmd, cpu_time()
			});
			break;
		}
	} while (iter++ != rob_.end());
	if (rob.busy) return;
	switch (rob.cmd & 127) {
	case Instr_Branch:
		predictor_->revise(rob.addr, rob.dest_reg, rob.val);
		if (rob.dest_reg != rob.val) {
			if (rob.val)
				PC = rob.addr + parse_imm(rob.cmd, 'B');
			else PC = rob.addr + 4;
			mispredicted_ = true;
			return;
		}
		else rob_.pop();
		break;
	case Instr_S:
		val = reg[parse_rs2(rob.cmd)];
		switch (parse_func3(rob.cmd)) {
		case 0: mu_.pmem->writeBytes(rob.val, &val, 1); break;
		case 1: mu_.pmem->writeBytes(rob.val, &val, 2); break;
		case 2: mu_.pmem->writeBytes(rob.val, &val, 4); break;
		}
		rob_.pop();
		break;
	default:
		reg[rob.dest_reg] = rob.val;
		if (rob.Q == reg_file[rob.dest_reg].nxt_data)
			reg_file[rob.dest_reg] = 0;
		rob_.pop();
		break;
	}
}

bool TomasuloCPUCore::try_get_data_(int8 r, int32& val) {
	if (!reg_file[r]) {
		val = reg[r];
		return true;
	}
	ReorderBuffer::Entry& rob = rob_[reg_file[r]];
	if (!rob.busy) {
		val = rob.val;
		return true;
	}
	return false;
}

ResrvStation* TomasuloCPUCore::try_get_alu_() {
	for (auto& iter : alus_) {
		if (iter.state == RS_Idle &&
			iter.state.nxt_data == RS_Idle)
			return &iter;
	}
	return nullptr;
}

bool TomasuloCPUCore::load_rs_data_(ResrvStation& rs, int8 rs1, int8 rs2) {
	int32 tmp_val;
	bool flag = true;
	if (rs1) {
		if (try_get_data_(rs1, tmp_val)) {
			rs.V1 = tmp_val;
			rs.Q1 = 0;
		}
		else {
			rs.Q1 = reg_file[rs1];
			rs.V1 = 0;
			flag = false;
		}
	}
	else {
		rs.Q1 = 0;
		rs.V1 = 0;
	}
	if (rs2) {
		if (try_get_data_(rs2, tmp_val)) {
			rs.V2 = tmp_val;
			rs.Q2 = 0;
		}
		else {
			rs.Q2 = reg_file[rs2];
			rs.V2 = 0;
			flag = false;
		}
	}
	else {
		rs.Q2 = 0;
		rs.V2 = 0;
	}
	return flag;
}

void MemUnit::execute(ComnDataBus* cdb, size_t time) {
	if (load_queue.empty() || !cdb) return;
	if (time < load_queue.front().time + MEM_LOAD_TIME) return;
	uint32 val = 0;
	load_instr& instr = load_queue.front();
	cdb->busy = true;
	cdb->Q = instr.Q;
	switch (parse_func3(instr.cmd)) {
	case 0:
		pmem->readBytes(instr.addr, &val, 1);
		cdb->val = extend_sign(val, 8);
		break;
	case 1:
		pmem->readBytes(instr.addr, &val, 2);
		cdb->val = extend_sign(val, 16);
		break;
	case 2:
		pmem->readBytes(instr.addr, &val, 4);
		cdb->val = val;
		break;
	case 4:
		pmem->readBytes(instr.addr, &val, 1);
		cdb->val = val;
		break;
	case 5:
		pmem->readBytes(instr.addr, &val, 2);
		cdb->val = val;
		break;
	}
	load_queue.pop();
}
