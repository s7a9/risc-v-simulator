#include "TomasuloCore.h"
#include "MemoryLoader.h"
#include <iostream>

TomasuloCPUCore* cpu;
Predictor* predictor;
RvMemory* memory;

int main() {
	memory = new RvMemory(10);
	predictor = new TwoBitSaturatePredictor(1024);
	cpu = new TomasuloCPUCore(memory, predictor, 2, 2, 16);
	load_memory_from_file(memory, "D:\\SJTU\\2023Spring\\PPCA\\risc-v-simulator\\testcases\\basicopt1.data");
	while (!cpu->end_simulate) {
		cpu->tick();
	}
	std::cout << predictor->stat[0][0] << '/' << predictor->stat[0][1] << '\n';
	std::cout << predictor->stat[1][0] << '/' << predictor->stat[1][1] << '\n';
}