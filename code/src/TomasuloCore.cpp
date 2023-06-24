#include <cstring>
#include "TomasuloCore.h"

#define parse_op(cmd)		(int8)(cmd & 127)
#define parse_rd(cmd)		(int8)((cmd >> 7 ) & 31)
#define parse_rs1(cmd)		(int8)((cmd >> 15) & 31)
#define parse_rs2(cmd)		(int8)((cmd >> 20) & 31)
#define parse_func3(cmd)	(int8)((cmd >> 12) & 7)

int32 extend_sign(uint32 x, int8 bitc) {
	if (bitc && (x >> (bitc - 1))) {
		uint32 mask = 1u << bitc;
		for (int i = bitc; i < 32; ++i) {
			x |= mask;
			mask <<= 1;
		}
	}
	return x;
}

int32 parse_imm(uint32 cmd, char type) {
	switch (type)
	{
	case 'I': return extend_sign(
			cmd >> 20, 12);
	case 'S': return extend_sign(
			((cmd & 0xFE000000) >> 20) +
			((cmd & 0xF80) >> 7), 12);
	case 'B': return extend_sign(
			((cmd & 0x80000000) >> 19) +
			((cmd & 0x7E000000) >> 20) +
			((cmd & 0x00000F00) >> 7) +
			((cmd & 0x00000080) << 4), 12);
	case 'U': return cmd & 0xFFFFF000;
	case 'J': return extend_sign(
			((cmd & 0x80000000) >> 11) +
			((cmd & 0x7FE00000) >> 20) +
			((cmd & 0x00100000) >> 9) +
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
	issue_();
	execute_();
	write_result_();
	commit_();
	for (auto& iter : alus_) iter.rs.tick();
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
	cdbs_.resize(cdbn);
	alus_.resize(alun);
}

void TomasuloCPUCore::issue_() {
	// Reoerder Buffer is Full, cannot issue instruction
	if (rob_.full()) return;
	ReorderBuffer::Entry& rob = rob_.push();
	int8 rob_q = rob.Q;
	ResrvStation* rs = nullptr;
	int i = 0;
	int32 tmp_val = 0;
	uint32 cmd;
	mu_.pmem->read(PC, cmd);
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
		reg_file[rd] = rob_q;
		rob.val = parse_imm(cmd, 'U');
		break;
	case Instr_AUIPC:
		rob.busy = false;
		reg_file[rd] = rob_q;
		rob.val = parse_imm(cmd, 'U') + PC;
		break;
	case Instr_JAL:
		rob.busy = false;
		reg_file[rd] = rob_q;
		rob.val = PC + 4;
		PC = PC + parse_imm(cmd, 'J');
		break;
	case Instr_JALR:
		if (try_get_data_(parse_rs1(cmd), tmp_val)) {
			rob.busy = false;
			reg_file[rd] = rob_q;
			rob.val = PC + 4;
			PC = (tmp_val + (parse_imm(cmd, 'I') << 1)) & 0xFFFFFFFE;
		}
		else { 
			// Failed to get data from registers or ROB
			// issue MUST be STALLED and wait for rs1 to be calculated
			// OR cpu would get wrong PC
			PC = PC;
			rob_.withdraw();
		}
		break;
	case Instr_Branch:
		if (!try_get_alu_(i)) {
			PC = PC;
			rob_.withdraw();
			break;
		}
		rob.busy = true;
		rs = &alus_[i].rs;
		rs->cmd = cmd; rs->Dst = rob_q;
		rs->Op = parse_func3(cmd);
		rs->state = load_rs_data_(*rs, parse_rs1(cmd), parse_rs2(cmd)) ?
			RS_Ready : RS_Waiting;
		if (predictor_->predict(PC)) {
			PC = PC + parse_imm(cmd, 'B');
			rob.dest_reg = 1;
		}
		else {
			rob.dest_reg = 0;
		}
		break;
	case Instr_L:
		if (!try_get_alu_(i) || !rd) {
			PC = PC;
			rob_.withdraw();
			break;
		}
		rob.busy = true;
		rob.val = 0;
		reg_file[rd] = rob_q;
		rs = &alus_[i].rs;
		rs->cmd = cmd;
		rs->Dst = rob_q | 32;
		rs->Op = 0;
		rs->state = load_rs_data_(*rs, parse_rs1(cmd), 0) ?
			RS_Ready : RS_Waiting;
		rs->V2 = parse_imm(cmd, 'I');
		break;
	case Instr_S:
		if (!try_get_alu_(i)) {
			// Stall
			PC = PC;
			rob_.withdraw();
			break;
		}
		rob.dest_reg = 0;
		rob.busy = true;
		rs = &alus_[i].rs;
		rs->cmd = cmd; rs->Dst = rob_q;
		rs->Op = 0;
		rs->state = load_rs_data_(*rs, parse_rs1(cmd), 0) ?
			RS_Ready : RS_Waiting;
		rs->V2 = parse_imm(cmd, 'S');
		break;
	case Instr_ALUI:
		if (!try_get_alu_(i)) {
			PC = PC;
			rob_.withdraw();
			break;
		}
		rob.busy = true;
		reg_file[rd] = rob_q;
		rs = &alus_[i].rs;
		rs->cmd = cmd; rs->Dst = rob_q;
		rs->Op = parse_func3(cmd);
		rs->state = load_rs_data_(*rs, parse_rs1(cmd), 0) ?
			RS_Ready : RS_Waiting;
		rs->V2 = parse_imm(cmd, 'I');
		break;
	case Instr_ALU:
		if (!try_get_alu_(i)) {
			PC = PC;
			rob_.withdraw();
			break;
		}
		rob.busy = true;
		reg_file[rd] = rob_q;
		rs = &alus_[i].rs;
		rs->cmd = cmd; rs->Dst = rob_q;
		rs->Op = parse_func3(cmd);
		rs->state = load_rs_data_(*rs, parse_rs1(cmd), parse_rs2(cmd)) ?
			RS_Ready : RS_Waiting;
		break;
	}
}

void TomasuloCPUCore::execute_() {
	for (auto& iter : cdbs_) iter.busy = false; 
	mu_.execute(get_idle_cdb_(), cpu_time_);
	for (auto& iter : alus_) {
		ComnDataBus* cdb = get_idle_cdb_();
		if (!cdb) break;
		iter.execute(cdb);
	}
}

void TomasuloCPUCore::write_result_() {
	for (auto& iter : alus_) {
		ResrvStation& rs = iter.rs;
		rs.execute(cdbs_);
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
	rob_.execute(cdbs_);
}

void TomasuloCPUCore::commit_() {
	if (rob_.empty()) return;
	ReorderBuffer::Entry& rob = rob_.front();
	int32 val = 0;
	auto iter = rob_.begin();
	do {
		uint32 opcode = iter->cmd & 127;
		if (opcode == Instr_S) break;
		if (opcode == Instr_L && iter->val == 1) {
			iter->val = 0;
			mu_.load_queue.push(MemUnit::load_instr{
				iter->Q, iter->addr, iter->cmd, cpu_time_
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
			rob_.clear();
			memset(reg_file, 0, sizeof(reg_file));
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
		if (rob.Q == reg_file[rob.dest_reg])
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

bool TomasuloCPUCore::try_get_alu_(int& i) {
	for (i = 0; i < alun_; ++i) {
		if (alus_[i].rs.state == RS_Empty ||
			alus_[i].rs.state == RS_Committed) {
			return true;
		}
	}
	return false;
}

bool TomasuloCPUCore::load_rs_data_(ResrvStation& rs, int8 rs1, int8 rs2) {
	int32 tmp_val;
	bool f1 = true, f2 = true;
	if (rs1) {
		if (try_get_data_(rs1, tmp_val)) {
			rs.V1 = tmp_val;
			rs.Q1 = 0;
		}
		else {
			rs.Q1 = reg_file[rs1];
			rs.V1 = 0;
			f1 = false;
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
			f2 = false;
		}
	}
	else {
		rs.Q2 = 0;
		rs.V2 = 0;
	}
	return f1 && f2;
}

void MemUnit::execute(ComnDataBus* cdb, size_t time) {
	if (load_queue.empty() || !cdb) return;
	if (time < load_queue.front().time + MEM_LOAD_TIME) return;
	uint32 val;
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
	case 2: case 4:
		pmem->readBytes(instr.addr, &val, 4);
		cdb->val = val;
		break;
	case 5:
		pmem->readBytes(instr.addr, &val, 2);
		cdb->val = val;
		break;
	}
	load_queue.pop();
}
