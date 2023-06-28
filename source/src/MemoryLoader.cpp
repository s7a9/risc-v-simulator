#include "MemoryLoader.h"

#include <iostream>
#include <fstream>
#include <string>
#include <cctype>


void load_memory_from_file(RvMemory* mem, const char* filename) {
    std::fstream file(filename, std::ios::in);
    if (!file.is_open()) {
        std::cerr << "Cannot open file \"" << filename << "\"" << std::endl;
        exit(-1);
    }
    load_memory(mem, file);
    file.close();
}

void load_memory(RvMemory* mem, std::istream& file) {
    std::string content;
    uint32 pos = 0;
    int8 x = 0;
    char* p;
    while (file >> content) {
        if (content[0] == '@') {
            pos = strtol(content.c_str() + 1, &p, 16);
        }
        else {
            x = strtol(content.c_str(), &p, 16);
            mem->write(pos, x);
            ++pos;
        }
    }
}
