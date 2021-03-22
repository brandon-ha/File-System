#include "fs.hpp"

// setup functions

// reset disk
void FileSystem::setup_disk() {
    // set fds
    unsigned char* empty_fds_block;
    int empty_fds_block_content[block_size/sizeof(int)];
    for (int i = 0; i < block_size/(int) sizeof(int); i++) {
        empty_fds_block_content[i] = -1;
    }
    empty_fds_block = (unsigned char *)&empty_fds_block_content;

    for (int i = 1; i <= fd_blocks; i++) {
        disk.write_block(i, empty_fds_block);
    }
    
    // set blocks
    for (int i = fd_blocks + 1; i < num_blocks; i++) {
        unsigned char empty[block_size];
        memset(empty, '\0', block_size);
        disk.write_block(i, empty);
    }
}

// reset OFT
void FileSystem::setup_OFT() {
    // get oft
    open_file_entry* current = oft;

    // set free entries
    for (int i = 0; i < oft_size; i++) {
        open_file_entry new_entry;

        current[i] = new_entry;
    }

    // set first pos to 0 
    current[0].pos = 0;
}

// reset bitmap
void FileSystem::setup_bitmap() {
    unsigned char current[block_size];
    memset(current, 0, block_size);

    // set first bits 0 - 6 to 1 
    for (int i = 0; i < 1 + fd_blocks; i++) {
        current[0] = current[0] | mask[i];
    }

    disk.write_block(0, current);
}

// set directory 
void FileSystem::setup_directory() {
    // update bitmap
    unsigned char bitmap[block_size];

    disk.read_block(0, bitmap);
    bitmap[0] = bitmap[0] | mask[1 + fd_blocks];

    disk.write_block(0, bitmap);

    // update fds
    unsigned char block_1[block_size]; 

    disk.read_block(1, block_1);

    fd dir_fd;
    dir_fd.size = 0;
    dir_fd.block_numbers[0] = 1 + fd_blocks;

    unsigned char* dir_fd_char = (unsigned char *) &dir_fd;

    memcpy(block_1, dir_fd_char, sizeof(fd));

    disk.write_block(1, block_1);

    // update block 7
    write_new_dir_block(7);
    
    // update OFT

    open_file_entry dir_entry;
    disk.read_block(7, dir_entry.buffer);
    dir_entry.pos = 0;
    dir_entry.fd_index = 0;
    oft[0] = dir_entry;
}

// fill cache with values from disk
void FileSystem::fill_cache() {
    disk.read_block(0, cache.bitmap);
    
    for (int i = 1; i <= fd_blocks; i++) {
        disk.read_block(i, (unsigned char *) &cache.file_descriptors[(fd_max_count / fd_blocks) * (i - 1)]);
    }
}