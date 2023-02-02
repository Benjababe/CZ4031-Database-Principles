#include "block.h"

#define BLOCK_SIZE 200

Block::Block()
{
    this->data = std::vector<char>(BLOCK_SIZE, '\0');
    this->record_count = 0;
}

/**
 * @brief Inserts record into the block
 *
 * @param block_offset Address offset to insert record
 * @param record Record to insert
 */
void Block::write_record(size_t block_offset, Record *record)
{
    std::memcpy(&this->data[block_offset], record, sizeof(Record));
    this->inc_record_count();
}

/**
 * @brief Reads record based off address offset
 *
 * @param block_offset Address offset to read from
 * @return Record
 */
Record Block::read_record(size_t block_offset)
{
    Record record;
    std::memcpy(&record, &this->data[block_offset], sizeof(Record));
    return record;
}

void Block::inc_record_count()
{
    ++this->record_count;
}