#ifndef RV_MEMORY_H
#define RV_MEMORY_H

#include <cstddef>
#include <map>
#include "Types.h"

class RvMemory {
private:
    const size_t block_size_, block_len_;

    std::map<size_t, char*> data_;

    char* get_mem_(uint32 idx);

public:
    // block_size in 2's exponential: 1 for 2, 10 for 1024
    RvMemory(size_t block_size);

    ~RvMemory();

    void readBytes(uint32 pos, char* buf, size_t size);

    void writeBytes(uint32 pos, const char* buf, size_t size);

    template <class T> 
    void read(uint32 pos, T& elem) {
        readBytes(pos, reinterpret_cast<char*>(&elem), sizeof(T));
    }

    template <class T>
    void write(uint32 pos, const T& elem) {
        writeBytes(pos, reinterpret_cast<const char*>(&elem), sizeof(T));
    }
};

#endif // RV_MEMORY_H