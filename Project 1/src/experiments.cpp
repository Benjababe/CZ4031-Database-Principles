#include "experiments.h"

void experiment_1(std::vector<RecordPtr> &record_ptrs)
{
    RecordPtr last = record_ptrs[record_ptrs.size() - 1];

    std::cout << "Number of records: " << record_ptrs.size() << std::endl;
    std::cout << "Size of a record: " << sizeof(Record) << " bytes" << std::endl;
    std::cout << "Number of records in a block: " << 200 / sizeof(Record) << std::endl;
    std::cout << "Number of blocks used: " << last.block_id + 1 << std::endl;
}