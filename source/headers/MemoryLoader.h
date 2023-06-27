#include "RvMemory.h"

#include <istream>

void load_memory_from_file(RvMemory* mem, const char* filename);

void load_memory(RvMemory* mem, std::istream& file);