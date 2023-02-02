#ifndef H_STRUCTS
#define H_STRUCTS

// size of record will be 20 bytes, allowing 10 records per block perfectly
struct Record
{
    char tconst[12];      // 12 bytes, fixed string length
    float average_rating; // 4 bytes
    int num_votes;        // 4 bytes
};

// keeps track of which block the record is stored in
// and where the record starts in the block
struct RecordPtr
{
    size_t block_id;
    size_t block_offset;
};

#endif