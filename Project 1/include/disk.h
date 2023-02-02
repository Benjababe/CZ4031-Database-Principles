#ifndef H_DISK
#define H_DISK

#include <cstdint>
#include <iostream>
#include <vector>

#include "structs.h"
#include "block.h"

class Disk
{
public:
    Disk();
    bool get_empty_address();
    RecordPtr add_record(Record *);
    Block read_block(size_t block_id);

private:
    size_t block_idx;    // points to the active block
    size_t block_offset; // points to an available empty space in the block
    size_t record_size;
    int memory_size, block_count;
    std::vector<Block> blocks;
};

#endif