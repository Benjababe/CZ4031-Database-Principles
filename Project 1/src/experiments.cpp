#include "experiments.h"

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
    std::cout << "parameter n of b+ tree : " << bpTree.maxKeys <<std::endl;
    std::cout << "number of nodes of b+ tree : " << bpTree.numNodes <<std::endl;
    std::cout << "number of levels of b+ tree : " << bpTree.height <<std::endl;
    Node* rootNode = bpTree.disk->getBlockPtr(bpTree.getRoot().block_id)->getNode();
    std::cout << "Keys of root node : ";
    for (int i=0; i< rootNode->numKeys; i++)
    {
        std::cout << rootNode->nodeKeyArr[i];
        if (i+1 != rootNode->numKeys)
            std::cout<<", ";
    }
    std::cout << std::endl;
}