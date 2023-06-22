#include <cstdlib>
#include <cstring>
#include "RvMemory.h"

RvMemory::RvMemory(size_t block_size):
    block_size_(block_size), 
    block_len_(1u << block_size_) {}

RvMemory::~RvMemory() {
    for (auto iter = data_.begin(); iter != data_.end(); ++iter) 
        free(iter->second);
}

char* RvMemory::get_mem_(uint32 idx) {
    char* mem = nullptr;
    auto result = data_.find(idx);
    if (result == data_.end()) {
        data_.insert(std::make_pair(idx, mem = (char*)malloc(block_len_)));
        memset(mem, 0, block_len_);
        return mem;
    }
    return result->second;
}

void RvMemory::readBytes(uint32 pos, char* buf, size_t size) {
    size_t block_idx = pos >> block_size_;
    size_t idx = pos & (block_len_ - 1u);
    size_t i = 0;
    char* mem_buf = nullptr;
    while (i < size) {
        if (mem_buf == nullptr)
            mem_buf = get_mem_(block_idx);
        if (idx + size <= block_len_ + i) {
            memcpy(buf + i, mem_buf + idx, size - i);
            break;
        }
        size_t tlen = block_len_ - idx;
        memcpy(buf + i, mem_buf + idx, tlen);
        i += tlen;
        idx = 0;
        ++block_idx;
        mem_buf = nullptr;
    }
}

void RvMemory::writeBytes(uint32 pos, const char* buf, size_t size) {
    size_t block_idx = pos >> block_size_;
    size_t idx = pos & (block_len_ - 1u);
    size_t i = 0;
    char* mem_buf = nullptr;
    while (i < size) {
        if (mem_buf == nullptr)
            mem_buf = get_mem_(block_idx);
        if (idx + size <= block_len_ + i) {
            memcpy(mem_buf + idx, buf + i, size - i);
            break;
        }
        size_t tlen = block_len_ - idx;
        memcpy(mem_buf + idx, buf + i, tlen);
        i += tlen;
        idx = 0;
        ++block_idx;
        mem_buf = nullptr;
    }
}