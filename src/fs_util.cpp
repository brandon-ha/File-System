#include "fs.hpp"

// util functions

// check if file with name filename already exists
bool FileSystem::exists(std::string filename) {
    open_file_entry* dir_oft_entry = &oft[0];
    fd* dir_fd = &cache.file_descriptors[0];
    directory_entry* curr_entry;

    for (int i = 0; i < fd_max_blocks; i++) {
        // hitting free means we went through all entries
        if (dir_fd->block_numbers[i] == FREE) {
            return false;
        } 

        seek(0, (i * block_size));

        for (int j = 0; j < block_size; j += ((int) sizeof(directory_entry))) {
            curr_entry = (directory_entry*) &dir_oft_entry->buffer[j];
            if (curr_entry->name == filename) {
                return true;
            }
        }
    }

    return false;
}

// gets next free file description
int FileSystem::get_free_fd(fd*& res) {
    fd* current = cache.file_descriptors;

    // fd 0 reserved for directory
    for (int i = 1; i < fd_max_count; i++) {
        if (current[i].size == FREE) {
            res = &current[i];
            return i;
        }
    }

    return -1;
}

// gets next free oft entry
int FileSystem::get_free_oft_entry(open_file_entry*& res) {
    for (int i = 0; i < oft_size; i++) {
        if (oft[i].pos == FREE) {
            res = &oft[i];
            return i;
        }
    }

    return -1;
}

// find next free block 
int FileSystem::get_free_block() {
    unsigned char* bitmap = cache.bitmap;
    int block_index = 0;
    for (int i = 0; i < num_blocks / (8 * (int) sizeof(char)); i++) {
        for (int j = 0; j < (8 * (int) sizeof(char)); j++, block_index++) {
            int val = bitmap[i] & mask[j];
            if (val == 0) {
                return block_index;
            }
        }
    }
    return -1;
}

// create new empty directory block at index index
int FileSystem::write_new_dir_block(int index) {
    if (index < 1 + fd_max_blocks || index >= num_blocks) {
        return -1;
    }
    directory_entry dir_obj;
    unsigned char block[block_size];

    for (int i = 0; i < block_size; i += ((int) sizeof(directory_entry))) {
        memcpy(&block[i], &dir_obj, sizeof(directory_entry));
    }

    disk.write_block(index, block);
    return 0;
}

// set bit index in bitmap (0 - num_blocks)
int FileSystem::set_bitmap_bit(int index, int val) {
    if (index < 0 || index >= num_blocks) {
        return -1;
    }

    unsigned char* bitmap = cache.bitmap;
    int j = index / 8;
    int i = index % 8;
    if (val == 1) {
        bitmap[j] = bitmap[j] | mask[i];
    } else {
        bitmap[j] = bitmap[j] & ~mask[i];
    }
    
    return 0;
}

// checks if block is free
bool FileSystem::is_free(int block_index) {
    unsigned char* bitmap = cache.bitmap;
    int j = block_index / 8;
    int i = block_index % 8;
    int val = bitmap[j] & mask[i];
    return val == 0;
}