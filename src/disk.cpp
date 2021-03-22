#include "disk.hpp"

// copies block b into buffer
int Disk::read_block(int block_index, unsigned char* to) {
    // input error checking
    if (block_index < 0 || block_index >= num_blocks) {
        return -1;
    }

    // copy over bytes from disk to buffer
    memcpy(to, &disk[block_index], block_size);
    return 0;
}

// copies buffer to disk block b
int Disk::write_block(int block_index, unsigned char* from) {
    // input error checking
    if (block_index < 0 || block_index >= num_blocks) {
        return -1;
    }

    // copy over bytes from buffer to disk
    memcpy(&disk[block_index], from, block_size);

    return block_index;
}
