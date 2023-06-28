#include "TomasuloCore.h"
#include "MemoryLoader.h"
#include <iostream>
#include <fstream>
#include <cstring>

TomasuloCPUCore* cpu;
Predictor* predictor;
RvMemory* memory;

const char* help_document = "A RISC-V RV32I Tomasulo CPU Simulator by s7a9\n\n"
	"usage: [-M mem_block_size] [-A alu_num] [-C cdb_num] [-R rob_num]\n"
	"       [-P always | alwaysNot | twobit size | local size | gshare size | tournament size]\n"
	"       [-stat] [input] [-o output]\n\n"
	"default configuration:\n"
	"mem_block_size=10, alu_num=4, cdb_num=2, rob_num=16\n";

size_t mem_argu = 10, alu_argu = 4, cdb_argu = 2, rob_argu = 16;

bool stat_on = false;
const char* input_filename = nullptr;
const char* output_filename = nullptr;

inline void print_result(std::ostream& out) {
	out << (cpu->simulate_result & 255);
	if (stat_on) {
		out << '\n' << cpu->cpu_time() << '\n';
		out << predictor->stat[0][0] << '\t' << predictor->stat[0][1] << '\n';
		out << predictor->stat[1][0] << '\t' << predictor->stat[1][1] << '\n';
	}
	out.flush();
}

Predictor* parse_predictor_config(int& argi, int argc, const char* argv[]) {
	if (argi == argc) {
		std::cerr << "Must indicate predictor type after -P\n";
		return nullptr;
	}
	if (strcmp(argv[argi], "always") == 0)
		return new AlwaysPredictor();
	else if (strcmp(argv[argi], "alwaysNot") == 0)
		return new AlwaysNotPredictor();
	if (++argi == argc) {
		std::cerr << "Must indicate predictor size for this type\n";
		return nullptr;
	}
	int size;
	if (!(size = atoi(argv[argi])) || size > 24) {
		std::cerr << "Illegal size for predictor\n";
		return nullptr;
	}
	if (strcmp(argv[argi - 1], "twobit") == 0) {
		return new TwoBitSaturatePredictor(size);
	}
	if (strcmp(argv[argi - 1], "local") == 0) {
		return new LocalPredictor(size);
	}
	if (strcmp(argv[argi - 1], "gshare") == 0) {
		return new GSharePredictor(size);
	}
	if (strcmp(argv[argi - 1], "tournament") == 0) {
		return new TournamentPredictor(size);
	}
	std::cerr << "Unknown predictor type!\n";
	return nullptr;
}

int main(int argc, const char* argv[]) {
	for (int argi = 1; argi < argc; ++argi) {
		if (strcmp(argv[argi], "-M") == 0) {
			if (++argi == argc) {
				std::cerr << "Must indicate memory size after -M\n";
				return -1;
			}
			if (!(mem_argu = atoi(argv[argi])) || mem_argu >= 20) {
				std::cerr << "Illegal memory size after -M\n";
				return -1;
			}
		}
		else if (strcmp(argv[argi], "-A") == 0) {
			if (++argi == argc) {
				std::cerr << "Must indicate ALU num after -A\n";
				return -1;
			}
			if (!(alu_argu = atoi(argv[argi])) || alu_argu > 16) {
				std::cerr << "Illegal ALU num after -A\n";
				return -1;
			}
		}
		else if (strcmp(argv[argi], "-C") == 0) {
			if (++argi == argc) {
				std::cerr << "Must indicate CDB num after -C\n";
				return -1;
			}
			if (!(cdb_argu = atoi(argv[argi])) || cdb_argu > 16) {
				std::cerr << "Illegal CDB num after -C\n";
				return -1;
			}
		}
		else if (strcmp(argv[argi], "-R") == 0) {
			if (++argi == argc) {
				std::cerr << "Must indicate ROB num after -R\n";
				return -1;
			}
			if (!(rob_argu = atoi(argv[argi])) || rob_argu > 16) {
				std::cerr << "Illegal ROB num after -R\n";
				return -1;
			}
		}
		else if (strcmp(argv[argi], "-o") == 0) {
			if (++argi == argc) {
				std::cerr << "Must indicate output filename after -o\n";
				return -1;
			}
			output_filename = argv[argi];
		}
		else if (strcmp(argv[argi], "-stat") == 0) {
			stat_on = true;
		}
		else if (strcmp(argv[argi], "-h") == 0) {
			std::cout << help_document;
			return 0;
		}
		else if (strcmp(argv[argi], "-P") == 0) {
			if (!(predictor = parse_predictor_config(++argi, argc, argv)))
				return -1;
		}
		else {
			if (input_filename) {
				std::cerr << "Unexpected arguemnt: " << argv[argi];
				return -1;
			}
			input_filename = argv[argi];
		}
	}
	memory = new RvMemory(mem_argu);
	if (!predictor) predictor = new LocalPredictor(16);
	cpu = new TomasuloCPUCore(memory, predictor, cdb_argu, alu_argu, rob_argu);
	if (input_filename) load_memory_from_file(memory, input_filename);
	else load_memory(memory, std::cin);
	while (!cpu->end_simulate) cpu->tick();
	if (output_filename) {
		std::ofstream output_file(output_filename, std::ios::out);
		if (!output_file.is_open()) {
			std::cerr << "Failed to write output file \"" << output_filename << "\"";
			print_result(std::cout);
		}
		else print_result(output_file);
	}
	else print_result(std::cout);
	delete cpu;
	delete predictor;
	delete memory;
}