#include <cmath>

#include "structs.h"
#include "bpTree.h"

#define BLOCK_SIZE 200

Node::Node(int maxKeys, bool isLeaf)
{
    this->isLeaf = isLeaf;
    this->nodeRecordPtrArr.resize(maxKeys + 1, std::vector<RecordPtr>(1));
    this->nodeKeyArr.resize(maxKeys, 0);
    this->numKeys = 0;
}

BPTree::BPTree(std::vector<RecordPtr> &record_ptrs, Disk &disk)
{
    this->disk = &disk;
    this->numNodes = 0;
    this->height = 0;
    // caculate max keys in a node
    this->maxKeys = (BLOCK_SIZE - sizeof(RecordPtr)) / (sizeof(int) + sizeof(RecordPtr));
}

/*
1. search for node to insert
2. if no node, create new node
3. if node is full, split into two nodes, keys in left > right (leaf nodes), keys in left = right (non-leaf nodes)
4. check if node have parent
5. if parent exists, update parent else create parent
6. start from step 3 until a parent is found that is not full
*/

/**
 * @brief Insert key into b+ tree
 *
 * @param numVotes key to insert
 * @param recordPointer record pointer
 */
void BPTree::insert(int numVotes, RecordPtr recordPointer)
{
    // No node exists
    if (this->root.block_id == 0)
    {
        Node *rootNode = new Node(this->maxKeys, true);
        rootNode->nodeRecordPtrArr[0][0] = recordPointer;
        rootNode->nodeKeyArr[0] = numVotes;
        rootNode->numKeys = 1;
        RecordPtr rootPointer = this->disk->addNode(rootNode);
        root = rootPointer;
        this->numNodes++;
        this->height++;
    }
    // At least one node exists
    else
    {
        RecordPtr parent = this->root;
        Block *cur = this->disk->getBlockPtr(parent.block_id);
        Node *curNode = cur->getNode();
        RecordPtr nextNodeLocation = parent;
        int lastNodeIndex = 0;
        std::vector<RecordPtr> parentRecordPtr;

        // search for node to put key, end if curNode is a leaf node
        while (!curNode->isLeaf)
        {
            lastNodeIndex = this->getIndexToInsert(numVotes, curNode);
            parentRecordPtr.push_back(parent);
            nextNodeLocation = curNode->nodeRecordPtrArr[lastNodeIndex][0];
            cur = this->disk->getBlockPtr(nextNodeLocation.block_id);
            curNode = cur->getNode();
            if (!curNode->isLeaf)
                parent = nextNodeLocation;
        }

        // check for duplicate key in leaf node
        for (int i = 0; i < curNode->numKeys; i++)
        {
            if (numVotes == curNode->nodeKeyArr[i])
            {
                curNode->nodeRecordPtrArr[i].push_back(recordPointer);
                return;
            }
        }

        int nodeIndex = this->getIndexToInsert(numVotes, curNode);

        // current leaf node is full, split current node to two, number of keys in left side > right side after adding new key
        if (curNode->numKeys + 1 > this->maxKeys)
        {

            Node *newLeafNode = new Node(this->maxKeys, true);
            RecordPtr newLeafNodePointer = this->disk->addNode(newLeafNode);
            this->numNodes++;
            int minKeys = floor((this->maxKeys + 1) / 2); // same for leaf and non-leaf node

            // new key inserted in first half (curNode)
            if (nodeIndex <= minKeys)
            {
                for (int i = minKeys; i < this->maxKeys; i++)
                {
                    newLeafNode->nodeKeyArr[i - minKeys] = curNode->nodeKeyArr[i];
                    newLeafNode->nodeRecordPtrArr[i - minKeys] = curNode->nodeRecordPtrArr[i];
                    curNode->nodeKeyArr[i] = 0;
                    curNode->nodeRecordPtrArr[i][0].block_id = 0;
                    curNode->nodeRecordPtrArr[i][0].block_offset = 0;
                }
                newLeafNode->numKeys = minKeys;
                curNode->numKeys = minKeys;

                // sort before adding
                this->sortLeafNode(curNode, nodeIndex);
                curNode->nodeKeyArr[nodeIndex] = numVotes;
                curNode->nodeRecordPtrArr[nodeIndex][0] = recordPointer;
            }
            // new key inserted in second half (newNode)
            else
            {
                for (int i = minKeys + 1; i < this->maxKeys; i++)
                {
                    newLeafNode->nodeKeyArr[i - (minKeys + 1)] = curNode->nodeKeyArr[i];
                    newLeafNode->nodeRecordPtrArr[i - (minKeys + 1)] = curNode->nodeRecordPtrArr[i];
                    curNode->nodeKeyArr[i] = 0;
                    curNode->nodeRecordPtrArr[i][0].block_id = 0;
                    curNode->nodeRecordPtrArr[i][0].block_offset = 0;
                }
                newLeafNode->numKeys = minKeys - 1;
                curNode->numKeys = minKeys;

                // sort before adding
                this->sortLeafNode(newLeafNode, nodeIndex - (minKeys + 1));
                newLeafNode->nodeKeyArr[nodeIndex - (minKeys + 1)] = numVotes;
                newLeafNode->nodeRecordPtrArr[nodeIndex - (minKeys + 1)][0] = recordPointer;
                newLeafNode->numKeys++;
            }
            curNode->numKeys++;

            // link leaf nodes
            newLeafNode->nodeRecordPtrArr[this->maxKeys] = curNode->nodeRecordPtrArr[this->maxKeys];
            curNode->nodeRecordPtrArr[this->maxKeys][0] = newLeafNodePointer;

            // check if current node has parent

            // current node no parent, create new parent node
            if (curNode->isLeaf && parentRecordPtr.size() == 0)
            {
                Node *newParentNode = new Node(this->maxKeys, false);
                newParentNode->nodeRecordPtrArr[0][0] = this->root;
                newParentNode->nodeRecordPtrArr[1][0] = newLeafNodePointer;
                newParentNode->nodeKeyArr[0] = newLeafNode->nodeKeyArr[0];
                newParentNode->numKeys = 1;
                RecordPtr newParentNodePointer = this->disk->addNode(newParentNode);
                this->root = newParentNodePointer;
                this->height++;
                this->numNodes++;
            }

            // current node has parent, update parent node
            else
            {
                Node *parentNode = this->disk->getBlockPtr(parent.block_id)->getNode();
                updateParentNode(parentNode, newLeafNode, lastNodeIndex, minKeys, parent, newLeafNodePointer, parentRecordPtr);
            }
        }

        // current leaf node has space to put key
        else
        {
            // sort keys then add new key

            // new key smaller than some/all keys, shift all larger keys to right
            this->sortLeafNode(curNode, nodeIndex);

            // new key larger than all keys, no shifting required
            curNode->nodeKeyArr[nodeIndex] = numVotes;
            curNode->nodeRecordPtrArr[nodeIndex].clear();
            curNode->nodeRecordPtrArr[nodeIndex].push_back(recordPointer);
            curNode->numKeys++;
        }
    }
}

/**
 * @brief Update parent node recursively
 *
 * @param parentNode parent node
 * @param newNode new node created
 * @param lastNodeIndex index to insert new node
 * @param minKeys minimum keys in a non-leaf node
 * @param parentPointer RecordPtr of parent node
 * @param newChildPointer RecordPtr of new node
 * @param parentRecordPtr Array of RecordPtr of parent nodes from root node
 */
void BPTree::updateParentNode(Node *parentNode, Node *newNode, int lastNodeIndex, int minKeys, RecordPtr parentPointer,
                              RecordPtr newChildPointer, std::vector<RecordPtr> &parentRecordPtr)
{
    if (parentNode->numKeys + 1 > this->maxKeys)
    {
        // get recordPtr of current parentNode's parentNode
        int parentofParentNodeIndex = -1;
        for (int i = parentRecordPtr.size() - 1; i > 0; i--)
        {
            if (parentRecordPtr[i].block_id == parentPointer.block_id)
            {
                parentofParentNodeIndex = i - 1;
                break;
            }
        }

        RecordPtr parentNodeSiblingPointer = this->splitParentNode(parentNode, lastNodeIndex, minKeys, newChildPointer);

        // if parent node = root, create new root and update root
        if (parentPointer.block_id == this->root.block_id)
        {
            Node *newRootNode = new Node(this->maxKeys, false);
            RecordPtr newRootNodePointer = this->disk->addNode(newRootNode);
            this->numNodes++;
            this->height++;
            newRootNode->nodeRecordPtrArr[0][0] = parentPointer;
            newRootNode->nodeRecordPtrArr[1][0] = parentNodeSiblingPointer;
            newRootNode->nodeKeyArr[0] = this->getSmallestKeyInSubtree(parentNodeSiblingPointer);
            newRootNode->numKeys = 1;
            this->root = newRootNodePointer;
        }

        // parent node not root
        else
        {
            RecordPtr parentOfParentPointer = parentRecordPtr[parentofParentNodeIndex];
            Node *parentOfParentNode = this->disk->getBlockPtr(parentOfParentPointer.block_id)->getNode();
            int newlastNodeIndex;

            // get new index to insert
            for (int i = 0; i < this->maxKeys + 1; i++)
            {
                if (parentOfParentNode->nodeRecordPtrArr[i][0].block_id == parentPointer.block_id)
                {
                    newlastNodeIndex = i;
                    break;
                }
            }
            Node *parentNodeSibling = this->disk->getBlockPtr(parentNodeSiblingPointer.block_id)->getNode();
            this->updateParentNode(parentOfParentNode, parentNodeSibling, newlastNodeIndex, minKeys, parentOfParentPointer,
                                   parentNodeSiblingPointer, parentRecordPtr);
        }
    }

    // parent node has space
    else
    {
        for (int i = parentNode->numKeys; i > lastNodeIndex; i--)
        {
            parentNode->nodeRecordPtrArr[i + 1] = parentNode->nodeRecordPtrArr[i];
        }
        parentNode->nodeRecordPtrArr[lastNodeIndex + 1][0] = newChildPointer;
        parentNode->numKeys++;

        for (int i = 0; i < parentNode->numKeys; i++)
        {
            parentNode->nodeKeyArr[i] = this->getSmallestKeyInSubtree(parentNode->nodeRecordPtrArr[i + 1][0]);
        }
    }
}

/**
 * @brief Split parent nodes
 *
 * @param parentNode parent node
 * @param lastNodeIndex index to insert new node
 * @param minKeys minimum keys in a non-leaf node
 * @param newPointer new node created
 * @return RecordPtr to new parent node sibling
 */
RecordPtr BPTree::splitParentNode(Node *parentNode, int lastNodeIndex, int minKeys, RecordPtr newPointer)
{
    Node *parentNodeSibling = new Node(this->maxKeys, false);
    RecordPtr parentNodeSiblingPointer = this->disk->addNode(parentNodeSibling);
    this->numNodes++;

    int nonLeafminChild = minKeys + 1;

    if (lastNodeIndex < minKeys)
    {
        // update new node
        for (int i = 0; i < nonLeafminChild; i++)
        {
            parentNodeSibling->nodeRecordPtrArr[i] = parentNode->nodeRecordPtrArr[i + minKeys];

            // reset current node key and record
            parentNode->nodeRecordPtrArr[i + minKeys][0].block_id = 0;
            parentNode->nodeRecordPtrArr[i + minKeys][0].block_offset = 0;

            // check because 1 less key than pointer
            if ((i + minKeys) < parentNode->nodeKeyArr.size())
                parentNode->nodeKeyArr[i + minKeys] = 0;
        }

        // update current node
        for (int i = minKeys; i > lastNodeIndex + 1; i--)
        {
            parentNode->nodeRecordPtrArr[i] = parentNode->nodeRecordPtrArr[i - 1];
        }
        parentNode->nodeRecordPtrArr[lastNodeIndex + 1][0] = newPointer;

        // update keys
        for (int i = 0; i < minKeys; i++)
        {
            parentNode->nodeKeyArr[i] = this->getSmallestKeyInSubtree(parentNode->nodeRecordPtrArr[i + 1][0]);
            parentNodeSibling->nodeKeyArr[i] = this->getSmallestKeyInSubtree(parentNodeSibling->nodeRecordPtrArr[i + 1][0]);
        }
    }
    // lastNodeIndex > minkeys
    else
    {
        // update new node
        for (int i = 0; i < minKeys; i++)
        {
            parentNodeSibling->nodeRecordPtrArr[i] = parentNode->nodeRecordPtrArr[i + minKeys + 1];

            // reset current node key and record
            parentNode->nodeRecordPtrArr[i + minKeys + 1][0].block_id = 0;
            parentNode->nodeRecordPtrArr[i + minKeys + 1][0].block_offset = 0;
            parentNode->nodeKeyArr[i + minKeys] = 0;
        }

        // shift records
        for (int i = minKeys; i > lastNodeIndex - minKeys; i--)
        {
            parentNodeSibling->nodeRecordPtrArr[i] = parentNodeSibling->nodeRecordPtrArr[i - 1];
        }
        parentNodeSibling->nodeRecordPtrArr[lastNodeIndex - minKeys][0] = newPointer;

        // update keys
        for (int i = 0; i < minKeys; i++)
        {
            parentNodeSibling->nodeKeyArr[i] = this->getSmallestKeyInSubtree(parentNodeSibling->nodeRecordPtrArr[i + 1][0]);
        }
    }

    parentNodeSibling->numKeys = minKeys;
    parentNode->numKeys = minKeys;

    return parentNodeSiblingPointer;
}

/**
 * @brief Sort key and record of leaf node
 *
 * @param curNode current node
 * @param start start index
 * @param end end index
 */
void BPTree::sortLeafNode(Node *curNode, int end)
{
    for (int i = curNode->numKeys; i > end; i--)
    {
        curNode->nodeKeyArr[i] = curNode->nodeKeyArr[i - 1];
        curNode->nodeRecordPtrArr[i] = curNode->nodeRecordPtrArr[i - 1];
    }
}

/**
 * @brief Find index to insert into current node
 *
 * @param numVotes current key value
 * @param curNode current node
 * @return index to insert into current node
 */
int BPTree::getIndexToInsert(int numVotes, Node *curNode)
{
    int i = 0;
    for (i; i < curNode->numKeys; i++)
    {
        if (numVotes < curNode->nodeKeyArr[i])
            return i;
    }
    return i;
}

/**
 * @brief Find smallest key in sub tree
 *
 * @param parent parent node
 * @return smallest key
 */
int BPTree::getSmallestKeyInSubtree(RecordPtr parent)
{

    Node *curNode = this->disk->getBlockPtr(parent.block_id)->getNode();
    RecordPtr nextNode;
    while (!curNode->isLeaf)
    {
        nextNode = curNode->nodeRecordPtrArr[0][0];
        curNode = this->disk->getBlockPtr(nextNode.block_id)->getNode();
    }
    return curNode->nodeKeyArr[0];
}

/**
 * @brief Get root node
 *
 * @return recordPtr of root node
 */
RecordPtr BPTree::getRoot()
{
    return this->root;
}

/**
 * @brief Returns leaf node possibly containing the search value
 *
 * @param search_val
 * @param node_access_count Value to keep track of number of index nodes accessed
 * @return Node* pointer to the leaf node
 */
Node *BPTree::findLeafNode(int search_val, size_t &node_access_count)
{
    RecordPtr rootPtr = this->root;
    Block rootBlock = this->disk->read_block(rootPtr.block_id);
    Node *node = rootBlock.getNode();

    while (!node->isLeaf)
    {
        size_t i;
        for (i = 1; i <= node->numKeys; i++)
        {
            if (node->nodeKeyArr[i - 1] >= search_val)
                break;
        }
        if (node->nodeRecordPtrArr.size() == 0)
            int x = 0;
        RecordPtr ptr = node->nodeRecordPtrArr[i - 1][0];
        Block tmp_blk = this->disk->read_block(ptr.block_id);
        node = tmp_blk.getNode();
        ++node_access_count;
    }

    return node;
}

/**
 * @brief Get total number of leaf nodes
 *
 * @return number of leaf nodes
 */
int BPTree::getNumLeafNodes()
{
    int total = 0;
    Node *curNode = this->disk->getBlockPtr(this->root.block_id)->getNode();
    while (!curNode->isLeaf)
    {
        curNode = this->disk->getBlockPtr(curNode->nodeRecordPtrArr[0][0].block_id)->getNode();
    }
    RecordPtr nextNode = curNode->nodeRecordPtrArr[this->maxKeys][0];
    while (nextNode.block_id != 0)
    {
        total++;
        curNode = this->disk->getBlockPtr(nextNode.block_id)->getNode();
        nextNode = curNode->nodeRecordPtrArr[this->maxKeys][0];
    }
    total++;
    return total;
}

/**
 * @brief Delete key of B+ tree
 *
 * @param numVotes key to delete
 */
void BPTree::delete(int numVotes)
{
    if (this->root.block_id == 0) // tree is empty
    {
        std::cout << "The tree is empty" << std::endl;
        return;
    }
    else // tree is not empty so it must contain a root
    {
        RecordPtr parent = this->root;
        RecordPtr curPtr;
        Block *cur = this->disk->getBlockPtr(parent.block_id);
        Node *curNode = cur->getNode();
        Node *rootNode = curNode;
        int left, right;
        Node *parentNode;
        std::vector<RecordPtr> parentRecordPtr;
        // parentRecordPtr.push_back(parent);
        while (!curNode->isLeaf) // travel to the leaf node
        {
            for (int i = 0; i < curNode->numKeys; i++)
            {
                parentNode = curNode;
                left = i - 1;
                right = i + 1;

                if (numVotes < curNode->nodeKeyArr[i]) // key is less than a key value -> follow the pointer
                {
                    addLeftRight(left, right, parentRecordPtr);
                    curPtr = curNode->nodeRecordPtrArr[i][0];
                    parentRecordPtr.push_back(curPtr);
                    cur = this->disk->getBlockPtr(curPtr.block_id);
                    curNode = cur->getNode();
                    break;
                }
                if (i == curNode->numKeys - 1) // reach the last key pointer
                {
                    left = i;
                    right = i + 2;
                    addLeftRight(left, right, parentRecordPtr);
                    curPtr = curNode->nodeRecordPtrArr[i + 1][0];
                    parentRecordPtr.push_back(curPtr);
                    cur = this->disk->getBlockPtr(curPtr.block_id);
                    curNode = cur->getNode();
                    break;
                }
            }
        }
        // now we reached the leaf node
        // scan for the node to be deleted
        bool found = false;
        int pos;
        for (pos = 0; pos < curNode->numKeys; pos++)
        {
            if (curNode->nodeKeyArr[pos] == numVotes) // we found the key
            {
                found = true;
                break;
            }
        }
        if (!found) // we could not find the key
        {
            std::cout << "Key not found\n";
            return;
        }
        // we could find the key
        // update the key to the next key value
        for (int i = pos; i < curNode->numKeys - 1; i++)
        {
            curNode->nodeKeyArr[i] = curNode->nodeKeyArr[i + 1];
            curNode->nodeRecordPtrArr[i] = curNode->nodeRecordPtrArr[i + 1]
        }
        curNode->nodeKeyArr[i] = 0;
        curNode->nodeRecordPtrArr[i] = curNode->nodeRecordPtrArr[i + 1]; // transfer the final pointer
        curNode->nodeRecordPtrArr[i + 1] = std::vector<RecordPtr>(1);    // update last pointer to null pointer
        curNode->numKeys--;                                              // decrease node size

        // Case 1: number of keys on the node is still acceptable
        if (curNode->numKeys >= (maxKeys + 1) / 2)
        {
            return;
        }

        // retrieve the left node and right node (if any)
        // the order in the array: current node, right node, left node

        parentRecordPtr.pop_back(); // pop the current node because now we are in the current node already

        RecordPtr rightPtr = parentRecordPtr[parentRecordPtr.size() - 1];

        parentRecordPtr.pop_back(); // pop the right node

        RecordPtr leftPtr = parentRecordPtr[parentRecordPtr.size() - 1];

        parentRecordPtr.pop_back(); // pop the left node

        // Case 2: We can borrow from the left or right sibling

        // leftNode has enough nodes to share
        if (leftPtr.block_id != 0)
        {
            Block *left = this->disk->getBlockPtr(leftPtr.block_id);
            Node *leftNode = left->getNode();
            if (leftNode->numKeys >= (maxKeys + 1) / 2 + 1)
            {
                for (int i = curNode->numKeys; i > 0; i--)
                {
                    curNode->nodeKeyArr[i] = curNode->nodeKeyArr[i - 1];
                }
                for (int i = curNode->numKeys + 1; i > 0; i--)
                {
                    curNode->nodeRecordPtrArr[i] = curNode->nodeRecordPtrArr[i - 1];
                }
                curNode->numKeys++;
                curNode->nodeKeyArr[0] = leftNode->nodeKeyArr[leftNode->numKeys - 1];
                curNode->nodeRecordPtrArr[0] = leftNode->nodeRecordPtrArr[leftNode->numKeys - 1]; // transfer leaf
                leftNode->numKeys--;
                leftNode->nodeRecordPtrArr[leftNode->numKeys] = std::vector<RecordPtr>(1); // ptr to left sibling
                leftNode->nodeKeyArr[leftNode->numKeys] = 0;
                return;
            }
        }

        // rightNode has enough nodes to share
        if (rightPtr.block_id != 0)
        {
            Block *right = this->disk->getBlockPtr(rightPtr.block_id);
            Node *rightNode = right->getNode();
            if (rightNode->numKeys >= (maxKeys + 1) / 2 + 1)
            {
                for (int i = curNode->numKeys; i > 0; i--)
                {
                    curNode->nodeKeyArr[i] = curNode->nodeKeyArr[i - 1];
                }
                for (int i = curNode->numKeys + 1; i > 0; i--)
                {
                    curNode->nodeRecordPtrArr[i] = curNode->nodeRecordPtrArr[i - 1];
                }
                curNode->numKeys++; // ptr to next leaf
                curNode->nodeKeyArr[0] = rightNode->nodeKeyArr[rightNode->numKeys - 1];
                curNode->nodeRecordPtrArr[0] = rightNode->nodeRecordPtrArr[rightNode->numKeys - 1]; // transfer leaf
                rightNode->numKeys--;
                rightNode->nodeRecordPtrArr[rightNode->numKeys] = std::vector<RecordPtr>(1); // ptr to left sibling
                rightNode->nodeKeyArr[rightNode->numKeys] = 0;
                return;
            }
        }

        // Case 3: We need to merge two nodes
        if (leftPtr.block_id != 0)
        {
            for (int i = leftNode->numKeys, j = 0; j < curNode->numKeys; i++, j++)
            {
                leftNode->nodeKeyArr[i] = curNode->nodeKeyArr[j]; // move all keys to left sibling
                leftNode->nodeRecordPtrArr[i] = curNode->nodeRecordPtrArr[j];
            }
            lefNode->nodeRecordPtrArr[maxKeys + 1] = curNode->nodeRecordPtrArr[maxKeys + 1];
            leftNode->ptr[leftNode->size] = NULL;
            leftNode->numKeys += curNode->numKeys;
            FixInternal(parent->keys[left], parent, cursor); // fix the remaining internal nodes
            delete[] curNode->nodeKeyArr;
            delete[] curNode->nodeRecordPtrArr;
            delete curNode;
        }
        }
}

void BPTree::addLeftRight(int left, int right, std::vector<RecordPtr> parentRecordPtr)
{
    if (left < 0)
    {
        RecordPtr dummy;
        parentRecordPtr.push_back(dummy);
    }
    else
    {
        parentRecordPtr.push_back(curNode->nodeRecordPtrArr[left][0]);
    }
    if (right > curNode->numKeys)
    {
        RecordPtr dummy;
        parentRecordPtr.push_back(dummy);
    }
    else
    {
        parentRecordPtr.push_back(curNode->nodeRecordPtrArr[right][0]);
    }
}

void FixInternal()
{
}