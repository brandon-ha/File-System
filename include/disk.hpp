#include <cstring>

#pragma once

// params
constexpr int FREE = -1;
constexpr int num_blocks = 64;
constexpr int block_size = 512;

class Disk {
    private:
        unsigned char* disk[num_blocks][block_size];

    public:
        int read_block(int block_index, unsigned char* to);
        int write_block(int block_index, unsigned char* from);
};
