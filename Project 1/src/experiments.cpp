#include "experiments.h"

std::ostream &operator<<(std::ostream &os, const Record &record)
{
    return os << "tconst: " << record.tconst << "\trating: " << record.average_rating << "\tnum votes: " << record.num_votes;
}

void experiment_1(std::vector<RecordPtr> &record_ptrs)
{
    RecordPtr last = record_ptrs[record_ptrs.size() - 1];

    std::cout << "Number of records: " << record_ptrs.size() << std::endl;
    std::cout << "Size of a record: " << sizeof(Record) << " bytes" << std::endl;
    std::cout << "Number of records in a block: " << 200 / sizeof(Record) << std::endl;
    std::cout << "Number of blocks used: " << last.block_id + 1 << std::endl;
    std::cout << std::endl;
}

void experiment_2(BPTree &bpTree)
{
    std::cout << "parameter n of b+ tree : " << bpTree.maxKeys << std::endl;
    std::cout << "number of nodes of b+ tree : " << bpTree.numNodes << std::endl;
    std::cout << "number of levels of b+ tree : " << bpTree.height << std::endl;
    Node *rootNode = bpTree.disk->getBlockPtr(bpTree.getRoot().block_id)->getNode();
    std::cout << "Keys of root node : ";
    for (int i = 0; i < rootNode->numKeys; i++)
    {
        std::cout << rootNode->nodeKeyArr[i];
        if (i + 1 != rootNode->numKeys)
            std::cout << ", ";
    }
    std::cout << std::endl;
}

void experiment_3(Disk &disk, BPTree &bpTree)
{
    std::map<size_t, Block> data_block_cache; // keep track number of blocks read to "memory"
    size_t node_access_count = 0;             // no. of node blocks read
    float rating_sum = 0;

    RecordPtr rootPtr = bpTree.getRoot();
    Block rootBlock = disk.read_block(rootPtr.block_id);
    Node *node = rootBlock.getNode();

    // access root node
    ++node_access_count;

    int search_val = 500;
    std::vector<RecordPtr> result_ptrs;

    time_t start_time_index = get_current_time();

    while (!node->isLeaf)
    {
        size_t i;
        for (i = 1; i <= node->numKeys; i++)
        {
            if (node->nodeKeyArr[i - 1] >= search_val)
                break;
        }

        RecordPtr ptr = node->nodeRecordPtrArr[i - 1][0];
        Block tmp_blk = disk.read_block(ptr.block_id);
        node = tmp_blk.getNode();
        ++node_access_count;
    }

    // find the pointer to the records
    for (size_t i = 0; i < node->numKeys; i++)
    {
        if (node->nodeKeyArr[i] == search_val)
        {
            result_ptrs = node->nodeRecordPtrArr[i];
            break;
        }
    }

    for (RecordPtr ptr : result_ptrs)
    {
        Block result_blk;
        int blk_id = ptr.block_id;

        if (data_block_cache.count(blk_id))
        {
            result_blk = data_block_cache[blk_id];
        }
        else
        {
            result_blk = disk.read_block(blk_id);
            data_block_cache[blk_id] = result_blk;
        }

        Record result_rec = result_blk.read_record(ptr.block_offset);

        // print all records found
        // if (true)
        //     std::cout << result_rec << std::endl;

        rating_sum += result_rec.average_rating;
    }

    size_t blocks_accessed_linear = 0;
    size_t linear_result_count = 0;
    uint64_t time_taken_index = get_time_taken(start_time_index);

    time_t start_time_linear = get_current_time();

    Block block;
    for (size_t i = 0; i <= disk.get_block_idx(); ++i)
    {
        block = disk.read_block(i);
        if (block.get_record_count() == 0)
            break;
        ++blocks_accessed_linear;

        for (size_t r_i = 0; r_i < block.get_record_count(); ++r_i)
        {
            Record record = block.read_record(r_i * sizeof(Record));
            if (record.num_votes == 500)
                ++linear_result_count;
        }
    }

    uint64_t time_taken_linear = get_time_taken(start_time_linear);

    std::cout << "\nNumber of records with numVotes=500: " << result_ptrs.size() << std::endl;
    std::cout << "Number of index nodes accessed: " << node_access_count << std::endl;
    std::cout << "Number of data blocks accessed: " << data_block_cache.size() << std::endl;
    std::cout << "Average rating: " << rating_sum / result_ptrs.size() << std::endl;
    std::cout << "Time taken searching using B+ tree: " << time_taken_index << "ms" << std::endl;
    std::cout << "Number of data blocks accessed for linear scan: " << blocks_accessed_linear << std::endl;
    std::cout << "Time taken for linear scan: " << time_taken_linear << "ms" << std::endl;
}