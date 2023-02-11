#include <cstdint>
#include <fstream>
#include <iostream>
#include <string>
#include <sstream>

#include <algorithm>
#include <vector>

#include "disk.h"
#include "structs.h"
#include "block.h"
#include "experiments.h"
#include "bpTree.h"

void read_data_file(Disk &, std::vector<RecordPtr> &);
void createBPTree(Disk &, BPTree &, std::vector<RecordPtr> &);

int main()
{
    Disk disk;
    std::vector<RecordPtr> record_ptrs; // to be used for indexing
    read_data_file(disk, record_ptrs);

    experiment_1(record_ptrs);
    BPTree bpTree(record_ptrs, disk);
    createBPTree(disk, bpTree, record_ptrs);
    experiment_2(bpTree);
    return 0;
}

/**
 * @brief Reads data file and stores the records in the disk
 *
 * @param disk Object used to simulate storage
 * @param record_ptrs List of record pointers, used in B+ tree
 */
void read_data_file(Disk &disk, std::vector<RecordPtr> &record_ptrs)
{
    // read data.tsv or sample.tsv
    // sample contains only the first 50k lines so it doesn't
    // take forever to read & insert when testing features
    std::ifstream data_file("./data/sample.tsv");
    std::string line;

    // first getline() is to skip the header
    std::getline(data_file, line);

    // loop until EOF
    while (std::getline(data_file, line))
    {
        std::stringstream ss(line);
        Record record;

        // extracts line into record variables
        ss >> record.tconst >> record.average_rating >> record.num_votes;

        // adds record into disk and retrieves its starting address
        RecordPtr record_ptr = disk.add_record(&record);
        record_ptrs.push_back(record_ptr);
    }
}

/**
 * @brief create B+ Tree
 *
 * @param disk Disk 
 * @param bpTree BPTree 
 * @param record_ptrs Array of RecordPtr
 */
void createBPTree(Disk &disk, BPTree &bpTree, std::vector<RecordPtr> &record_ptrs)
{
    for (int i=0; i<record_ptrs.size(); i++)
    {
        RecordPtr recordPointer = record_ptrs[i];
        Record record = disk.getBlockPtr(recordPointer.block_id)->read_record(recordPointer.block_offset);
        bpTree.insert(record.num_votes, recordPointer);
    }
}