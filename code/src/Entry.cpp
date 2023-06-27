#include "TomasuloCore.h"
#include "MemoryLoader.h"
#include <iostream>

TomasuloCPUCore* cpu;
Predictor* predictor;
RvMemory* memory;

int main(int argc, const char* argv[]) {
	memory = new RvMemory(10);
	predictor = new TwoBitSaturatePredictor(1024);
	cpu = new TomasuloCPUCore(memory, predictor, 1, 1, 16);
	load_memory(memory, std::cin);
	//load_memory_from_file(memory, "D:\\SJTU\\2023Spring\\PPCA\\risc-v-simulator\\testcases\\pi.data");
	while (!cpu->end_simulate)
		cpu->tick();
	//std::cout.flush();
	//std::cout << "Cycles: " << cpu->cpu_time() << '\n' << "Predictor stat: \n";
	//std::cout << predictor->stat[0][0] << '\t' << predictor->stat[0][1] << '\n';
	//std::cout << predictor->stat[1][0] << '\t' << predictor->stat[1][1] << std::endl;
	delete cpu;
	delete predictor;
	delete memory;
}