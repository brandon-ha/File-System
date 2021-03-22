#include "fs.hpp"
#include "disk.hpp"

// file system constructor (automatically init when fs created)
FileSystem::FileSystem() {
    // init mask 
    mask[7] = 1;
    for (int i = 6; i >= 0; i--) {
        mask[i] = mask[i + 1] << 1;
    }

    init(false);
}

// file system destructor (save bitmap and file descriptors back to disk)
FileSystem::~FileSystem() {
    // save bitmap
    disk.write_block(0, cache.bitmap);
    
    // save fds
    for (int i = 0; i < fd_blocks; i++) {
        disk.write_block(i, (unsigned char *) &cache.file_descriptors[(fd_max_count / fd_blocks) * (i - 1)]);
    }

    // save blocks from oft buffers
    for (int i = 0; i < oft_size; i++) {
        open_file_entry* current = &oft[i];
        int current_ind = current->pos / block_size;
        fd* curr_fd = &cache.file_descriptors[current->fd_index];
        int block_num = curr_fd->block_numbers[current_ind];
        disk.write_block(block_num, current->buffer);
    }
}

// init file system from shell
int FileSystem::init() {
    return init(true);
}

// initialize file system
int FileSystem::init(bool display) {
    // clear M buffer
    memset(M, '\0', mem_buffer_size);

    // setup OFT, bitmap, and FDs
    setup_bitmap();
    setup_disk();
    setup_OFT();
    setup_directory();

    // set cache to disk contents
    fill_cache();

    if (display) {
        return 0;
    }
    return 1;
}

// create file with filename name
int FileSystem::create(std::string name) {
    // check if file exists 
    if (exists(name)) {
        return -1;
    }

    // search for free file desc
    fd* free_fd = 0;
    int fd_index = get_free_fd(free_fd);
    if (fd_index < 0) {
        return -1;
    }

    // check available space in directory
    open_file_entry* dir_oft_entry = &oft[0];
    if (dir_max_size - dir_oft_entry->size < (int) sizeof(directory_entry)) {
        return -1;
    }

    // update oft 
    oft->size += (int) sizeof(directory_entry);

    // search for free entry in directory
    directory_entry* curr_entry;
    fd* dir_fd = &cache.file_descriptors[0];

    // update dir fd (size)
    dir_fd->size += (int) sizeof(directory_entry);

    // assign file desc to file
    free_fd->size = 0;
    
    for (int i = 0; i < fd_max_blocks; i++) {
        // hitting free means we went through all entries
        if (dir_fd->block_numbers[i] == FREE) {
            // find next available disk block from bitmap
            int next_avail_block = get_free_block();
            // allocate new block
            dir_fd->block_numbers[i] = next_avail_block;
            write_new_dir_block(next_avail_block);
            set_bitmap_bit(next_avail_block, 1);
        } 

        seek(0, (i * block_size));

        for (int j = 0; j < block_size; j += ((int) sizeof(directory_entry))) {
            curr_entry = (directory_entry*) &dir_oft_entry->buffer[j];
            if (curr_entry->index == FREE) {
                // update directory disk block
                memcpy(curr_entry->name, name.c_str(), name.size());
                curr_entry->index = fd_index;
                return 0;
            }
        }
    }
    return -1;
}

// destroy file with filename name
int FileSystem::destroy(std::string name) {
    directory_entry* curr_entry;
    open_file_entry* dir_oft_entry = &oft[0];
    fd* dir_fd = &cache.file_descriptors[0];

    for (int i = 0; i < fd_max_blocks; i++) {
        if (dir_fd->block_numbers[i] == FREE) {
            break;
        } 

        seek(0, (i * block_size));

        for (int j = 0; j < block_size; j += ((int) sizeof(directory_entry))) {
            curr_entry = (directory_entry*) &dir_oft_entry->buffer[j];
            if (curr_entry->name == name) {
                // set fd size to FREE
                fd* file_fd = &cache.file_descriptors[curr_entry->index];
                file_fd->size = FREE;

                // mark all blocks as FREE and clear fd block nums
                for (int k = 0; k < fd_max_blocks; k++) {
                    if (file_fd->block_numbers[k] != FREE) {
                        set_bitmap_bit(file_fd->block_numbers[k], 0);
                        file_fd->block_numbers[k] = FREE;
                    }
                }

                // set dir entry name to '\0'
                memset(curr_entry->name, '\0', 4);

                // set dir entry index to FREE
                curr_entry->index = FREE;

                return 0;
            }
        }
    }
    return -1;
}

// open file with filename name
int FileSystem::open(std::string name) {
    // check if file exists
    if (!exists(name)) {
        return -1;
    }

    // search for free OFT entry
    open_file_entry* free_oft_entry;
    int oft_index = get_free_oft_entry(free_oft_entry);
    if (oft_index < 0) {
        return -1;
    }

    open_file_entry* dir_oft_entry = &oft[0];
    directory_entry* curr_entry;
    fd* dir_fd = &cache.file_descriptors[0];
    fd* target;

    for (int i = 0; i < fd_max_blocks; i++) {
        if (dir_fd->block_numbers[i] == FREE) {
            break;
        } 
        seek(0, (i * block_size));

        for (int j = 0; j < block_size; j += ((int) sizeof(directory_entry))) {
            curr_entry = (directory_entry*) &dir_oft_entry->buffer[j];
            if (curr_entry->name == name) {
                target = &cache.file_descriptors[curr_entry->index];
                break;
            }
        }
    }
    
    // check if file is already open
    for (int k = 0; k < oft_size; k++) {
        if (oft[k].fd_index == curr_entry->index) {
            if (oft[k].pos != FREE) {
                return -1;
            }
        }
    }

    // set values for oft entry
    free_oft_entry->pos = 0;
    free_oft_entry->size = target->size;
    free_oft_entry->fd_index = curr_entry->index;

    // assign block to block_numbers of fd
    int first_block = -1;
    if (free_oft_entry->size == 0) {
        first_block = get_free_block();
    } else {
        first_block = target->block_numbers[0];
    }
    
    // read to buffer
    disk.read_block(first_block, free_oft_entry->buffer);

    return oft_index;
}

// closes file with open file table index of index
int FileSystem::close(int index) {
    // check if index is in range (can't close directory)
    if (index <= 0 || index >= oft_size) {
        return -1;
    }
    
    // check if file is open at index in oft
    open_file_entry* oft_entry = &oft[index];
    if (oft_entry->pos == FREE) {
        return -1;
    }

    // write buffer to disk
    int current_block = oft_entry->pos / block_size;
    fd* curr_fd = &cache.file_descriptors[oft_entry->fd_index];
    disk.write_block(curr_fd->block_numbers[current_block], oft_entry->buffer);

    // update file size in fd
    curr_fd->size = oft_entry->size;

    // mark oft entry as free
    oft_entry->pos = FREE;

    return 0;
}

// copy count bytes from index to M[mem_pos]
int FileSystem::read(int index, int mem_pos, int count) {
    // check if index in range (block reads from directory)
    if (index <= 0 || index >= oft_size) {
        return -1;
    }
    // check if mem_pos in range 
    if (mem_pos < 0 || mem_pos >= block_size) {
        return -1;
    }

    // check if file open at index
    open_file_entry* oft_entry = &oft[index];
    if (oft_entry->pos == FREE) {
        return -1;
    }

    // copy from memory from buffer
    int bytes_read = 0;
    while (count > 0 && oft_entry->pos <= oft_entry->size) {
        // compute pos in buffer
        int buffer_pos = oft_entry->pos % block_size;

        // compute smaller value (versus buffer end - pos versus amt to reac left)
        int block_num = oft_entry->pos / block_size;
        int available_to_read = (oft_entry->size % block_size) - buffer_pos;
        int to_read = std::min(std::min(available_to_read, block_size - buffer_pos), count);

        if (available_to_read == 0) {
            break;
        }
        
        fd* file_fd = &cache.file_descriptors[oft_entry->fd_index];

        // check if end of buffer is reached
        if (buffer_pos == 0 && oft_entry->pos > 0) {
            // copy buffer to disk
            int curr_buffer_block = file_fd->block_numbers[block_num - 1];
            int next_buffer_block = file_fd->block_numbers[block_num];
            disk.write_block(curr_buffer_block, oft_entry->buffer);

            // copy next block from disk into buffer
            disk.read_block(next_buffer_block, oft_entry->buffer);
        }

        // copy bytes tp memory from buffer
        memcpy(&M[mem_pos + bytes_read], &oft_entry->buffer[buffer_pos], to_read);

        // update oft entry
        oft_entry->pos += to_read;

        // update count
        count -= to_read;

        // update actually read amount
        bytes_read += to_read;
    }

    return bytes_read;
}

// copy count bytes from M[mem_pos] to index
int FileSystem::write(int index, int mem_pos, int count) {
    // check if index in range (block writes to directory)
    if (index <= 0 || index >= oft_size) {
        return -1;
    }

    if (mem_pos < 0 || mem_pos >= block_size) {
        return -1;
    }

    // check if file open at i
    open_file_entry* oft_entry = &oft[index];
    if (oft_entry->pos == FREE) {
        return -1;
    }

    // check if file has blocks to write to yet 
    fd* file_fd = &cache.file_descriptors[oft_entry->fd_index];
    if (file_fd->block_numbers[0] == FREE) {
        int next_block = get_free_block();
        file_fd->block_numbers[0] = next_block;
        set_bitmap_bit(next_block, 1);
    }

    // copy from memory to buffer
    int written = 0;
    while (count > 0 && oft_entry->pos < (block_size * fd_max_blocks)) {
        // compute smaller value (block end - pos versus amt to write left)
        int block_num = (oft_entry->pos / block_size) + 1;
        int to_write = std::min((block_num * block_size) - oft_entry->pos, count);
        // compute pos in buffer
        int buffer_pos = oft_entry->pos % block_size;
        
        // check if end of buffer is reached
        if (buffer_pos == 0 && oft_entry->pos > 0) {
            // copy buffer to disk
            int curr_buffer_block = file_fd->block_numbers[block_num - 2];
            disk.write_block(curr_buffer_block, oft_entry->buffer);

            // copy next block from disk into buffer if free
            int next_block;
            if (curr_buffer_block != num_blocks - 1 && is_free(curr_buffer_block + 1)) {
                next_block = curr_buffer_block + 1;
            } else {
                if ((next_block = get_free_block()) < 0) {
                    return -1;
                }
            }
            disk.read_block(next_block, oft_entry->buffer);

            // update fd and bitmap
            file_fd->block_numbers[block_num - 1] = next_block;
            set_bitmap_bit(next_block, 1);
        }

        // copy bytes from memory to buffer
        for (int i = 0; i < to_write; i++) {
            if (isprint((int) M[mem_pos + i])) {
                oft_entry->buffer[buffer_pos + i] = M[mem_pos + i];

                // update pos
                oft_entry->pos++;

                // update size
                if (oft_entry->pos > oft_entry->size) {
                    oft_entry->size = oft_entry->pos;
                    file_fd->size = oft_entry->pos;
                }

                // update count
                count--;

                // update written count
                written++;
            } else {
                return written;
            }
        }
    }

    return written;
}

// change current position within an open file
int FileSystem::seek(int file, int pos) {
    // check if file in range
    if (file < 0 || file >= oft_size) {
        return -1;
    }
    
    open_file_entry* current = &oft[file];
    // check if file is free
    if (current->pos == FREE) {
        return -1;
    }
    // check if pos is in range
    if (pos < 0 || pos > current->size) {
        return -1;
    }

    // determine block b, with pos
    int block_ind = pos / block_size;
    int current_ind = current->pos / block_size;

    // if buffer doesn't contain block b
    if (block_ind != current_ind) {
        fd* curr_fd = &cache.file_descriptors[current->fd_index];
        // copy buffer into appropriate block on disk
        disk.write_block(curr_fd->block_numbers[current_ind], current->buffer);

        // copy block b from disk to buffer
        disk.read_block(curr_fd->block_numbers[block_ind], current->buffer);
    }
    // set current pos to pos
    current->pos = pos;

    return 0;
}

// write str to memory buffer[mem_pos]
int FileSystem::write_memory(int mem_pos, std::string str) {
    // check if mem_pos in range
    if (mem_pos < 0 || mem_pos >= block_size) {
        return -1;
    }

    // copy str to memory buffer
    memcpy(&M[mem_pos], str.c_str(), str.size());

    return str.size();
}

// read str to memory buffer[mem_pos]
std::pair<int, std::string> FileSystem::read_memory(int mem_pos, int num_bytes) {
    std::pair<int, std::string> result = {-1, ""};
    // check if mem_pos in range
    if (mem_pos < 0 || mem_pos >= block_size) {
        return result;
    }

    // check if num_bytes in range
    if (num_bytes < 0 || num_bytes > block_size) {
        return result;
    }

    // check if M + n - 1 in range
    if (mem_pos + num_bytes > block_size) {
        return result;
    }

    // copy str from memory buffer
    unsigned char temp[block_size];
    memset(temp, '\0', block_size);
    memcpy(&temp, &M[mem_pos], num_bytes);
    
    result.first = 0;
    result.second = (const char*) temp;

    return result;
}

// list files in directory
std::string FileSystem::list_directory() {
    std::string result;
    directory_entry* curr_entry;
    open_file_entry* dir_oft_entry = &oft[0];
    fd* dir_fd = &cache.file_descriptors[0];

    for (int i = 0; i < fd_max_blocks; i++) {
        if (dir_fd->block_numbers[i] == FREE) {
            break;
        } 

        seek(0, (i * block_size));

        for (int j = 0; j < block_size; j += ((int) sizeof(directory_entry))) {
            curr_entry = (directory_entry*) &dir_oft_entry->buffer[j];
            int size = cache.file_descriptors[curr_entry->index].size;
            if (curr_entry->index != FREE) {
                result += curr_entry->name;
                result += " ";
                result += std::to_string(size);
                result += " ";
            }
        }
    }

    return result.substr(0, result.size() - 1);
}