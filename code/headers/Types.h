#pragma once
#ifndef RV_TYPES_H
#define RV_TYPES_H

using int8      = char;
using int16     = short;
using int32     = int;
using uint16    = unsigned short;
using uint32    = unsigned int;

enum InstruType {
	Instr_LUI = 0b0110111,
	Instr_AUIPC = 0b0010111,
	Instr_JAL = 0b1101111,
	Instr_JALR = 0b1100111,
	Instr_Branch = 0b1100011,
	Instr_L = 0b0000011,
	Instr_S = 0b0100011,
	Instr_ALUI = 0b0010011,
	Instr_ALU = 0b0110011,
};

#endif // RV_TYPES_H