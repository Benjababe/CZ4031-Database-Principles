#ifndef H_BLOCK
#define H_BLOCK

#include <cstring>
#include <vector>

#include "structs.h"

class Block
{
public:
    Block();
    void write_record(size_t, Record *);
    Record read_record(size_t);
    void inc_record_count();

private:
    std::vector<char> data;
    size_t record_count;
};

#endif