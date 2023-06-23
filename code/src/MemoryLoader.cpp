#include "MemoryLoader.h"

#include <fstream>
#include <string>
#include <cctype>


void load_memory(RvMemory* mem, const char* filename) {
    std::fstream file(filename, std::ios::in);
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