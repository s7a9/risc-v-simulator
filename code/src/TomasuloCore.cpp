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
	case 'U': return extend_sign(
			cmd & 0xFFFFF000, 20);
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
}

TomasuloCPUCore::~TomasuloCPUCore() {}

void TomasuloCPUCore::tick() {
}

TomasuloCPUCore::TomasuloCPUCore(RvMemory* pmem,
	size_t cdbn, size_t alun, size_t robn):
	rob_(robn), cdbn_(cdbn), alun_(alun), robn_(robn) {
	mu_.pmem = pmem;
	cdbs_.reserve(cdbn);
	alus_.reserve(alun);
}

void TomasuloCPUCore::issue_() {
	// Reoerder Buffer is Full, cannot issue instruction
	if (rob_.full()) return;
	int rob_q = rob_.push();
	ReorderBuffer::Entry& rob = rob_.back();
	int i = 0;
	int32 tmp_val = 0;
	uint32 cmd;
	mu_.pmem->read(PC, cmd);
	int8 opcode = cmd & 127;
	int8 rd = parse_rd(cmd);
	PC = PC + 4;
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
			PC = (tmp_val + parse_imm(cmd, 'I') << 1) & 0xFFFFFFFE;
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
		break;
	case Instr_L:
		if (try_get_alu_(i)) {
			rob.busy = true;
			reg_file[rd] = rob_q;
			ResrvStation& rs = alus_[i].rs;
			rs.cmd = cmd;
			rs.Dst = rob_q;
		  	rs.Q1 = rs.Q2 = rs.Op = 0;
			rs.V2 = parse_imm(cmd, 'I');
			if (try_get_data_(parse_rs1(cmd), tmp_val)) {
				rs.state = RS_Ready;
				rs.V1 = tmp_val;
			}
			else {
				rs.state = RS_Waiting;
				rs.Q1 = reg_file[parse_rs1(cmd)];
				rs.V1 = 0;
			}
		}
		else { // Stall
			PC = PC;
			rob_.withdraw();
		}
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
		ResrvStation& rs = alus_[i].rs;
		rs.cmd = cmd; rs.Dst = rob_q;
		rs.Q1 = rs.Q2 = rs.Op = 0;
		rs.V2 = parse_imm(cmd, 'S');
		if (try_get_data_(parse_rs1(cmd), tmp_val)) {
			rs.state = RS_Ready;
			rs.V1 = tmp_val;
		}
		else {
			rs.state = RS_Waiting;
			rs.Q1 = reg_file[parse_rs1(cmd)];
			rs.V1 = 0;
		}
		break;
	case Instr_ALUI:
		if (!try_get_alu_(i)) {
			PC = PC;
			rob_.withdraw();
			break;
		}
		rob.busy = true;
		reg_file[rd] = rob_q;
		ResrvStation& rs = alus_[i].rs;
		rs.cmd = cmd; rs.Dst = rob_q;
		rs.Q1 = rs.Q2 = 0; rs.Op = parse_func3(cmd);
		rs.V2 = parse_imm(cmd, 'I');
		if (try_get_data_(parse_rs1(cmd), tmp_val)) {
			rs.state = RS_Ready;
			rs.V1 = tmp_val;
		}
		else {
			rs.state = RS_Waiting;
			rs.Q1 = reg_file[parse_rs1(cmd)];
			rs.V1 = 0;
		}
		break;
	case Instr_ALU:
		if (!try_get_alu_(i)) {
			PC = PC;
			rob_.withdraw();
			break;
		}
		rob.busy = true;
		reg_file[rd] = rob_q;
		ResrvStation& rs = alus_[i].rs;
		rs.cmd = cmd; rs.Dst = rob_q;
		rs.Op = parse_func3(cmd);
		if (try_get_data_(parse_rs1(cmd), tmp_val)) {
			rs.state = RS_Ready;
			rs.V1 = tmp_val;
		}
		else {
			rs.state = RS_Waiting;
			rs.Q1 = reg_file[parse_rs1(cmd)];
			rs.V1 = 0;
		}
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

