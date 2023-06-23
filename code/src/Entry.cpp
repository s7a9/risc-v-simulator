#include "TomasuloCore.h"
#include "MemoryLoader.h"

TomasuloCPUCore* cpu;
Predictor* predictor;
RvMemory* memory;

int main() {
	memory = new RvMemory(10);
	predictor = new TwoBitSaturatePredictor(1024);
	cpu = new TomasuloCPUCore(memory, predictor, 2, 2, 16);
	load_memory(memory, "D:\\SJTU\\2023Spring\\PPCA\\risc-v-simulator\\sample\\sample.data");
	while (true) {
		cpu->tick();
	}
}