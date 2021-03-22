#include <string>
#include <cstring>
#include <algorithm>
#include <iostream> // for debugging

#include "disk.hpp"

#pragma once

// params
constexpr int oft_size = 4;
constexpr int mem_buffer_size = 512;
constexpr int bitmap_size = 8;
constexpr int fd_blocks = 6;
constexpr int fd_max_blocks = 3;
constexpr int fd_max_count = 192;
constexpr int dir_max_size = mem_buffer_size * fd_max_blocks; // 1536 bytes

// file descriptor
typedef struct {
    int size = FREE; // -1 indicates descriptor
    int block_numbers[fd_max_blocks] = {FREE, FREE, FREE};
} fd;

// cache for the blocks 0 - 6 
typedef struct {
    unsigned char bitmap[block_size]; // only 64 bits used (or 8 bytes)
    fd file_descriptors[fd_max_count];
} fs_cache;

//directory
typedef struct {
    char name[4] = {'\0', '\0', '\0', '\0'};
    int index = FREE;
} directory_entry;

// open file table 
typedef struct {
    unsigned char buffer[block_size] = {'\0'}; // read/write buffer
    int pos = FREE; // current position (-1 = free)
    int size = 0; // file size
    int fd_index = FREE; // file descriptor index
} open_file_entry;

// functions
class FileSystem {
    private:
        Disk disk;
        unsigned char M[mem_buffer_size]; // memory buffer
        int mask[8]; // mask for manipulating bitmap
        fs_cache cache;
        open_file_entry oft[oft_size];

        bool exists(std::string filename);
        void setup_bitmap();
        void setup_disk();
        void setup_OFT();
        void setup_directory();
        void fill_cache();

        int get_free_fd(fd*& res);
        int get_free_oft_entry(open_file_entry*& res);
        int get_free_block();
        int write_new_dir_block(int index);
        int set_bitmap_bit(int index, int val);
        bool is_free(int block_index);

    public:
        FileSystem();
        ~FileSystem();
        int init();
        int init(bool display);
        int create(std::string name);
        int destroy(std::string name);
        int open(std::string name);
        int close(int index);
        int read(int index, int mem_pos, int count);
        int write(int index, int mem_pos, int count);
        int seek(int file, int pos);
        int write_memory(int mem_pos, std::string str);
        std::pair<int, std::string> read_memory(int mem_pos, int num_bytes);
        std::string list_directory();
};
