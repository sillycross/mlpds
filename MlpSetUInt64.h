#pragma once

#include "common.h"


namespace MlpSetUInt64
{

// Cuckoo hash table node
//
struct CuckooHashTableNode
{
	// root ======[parent]--child--*---------path-compression-string-------[this]--child-- ....... -- [minimum value in subtree]
	//                          indexLen                                 fullKeyLen                   8-byte minKey
	//
	// 2 bit: occupy flag, 00 = not used, 10 = used as node, 11 = used as bitmap
	// 3 bit: length of the indexing part of the key (1-8), indexLen in above diagram
	// 3 bit: length of the full key containing path-compressed bytes (1-8), fullKeyLen in above diagram
	// 3 bit: 000 = using internal map, 100 = pointer external map, otherwise offset of the external bitmap 
	// 3 bit: # of childs if using internal map, 0 + bitmap's highest 2 bits if using external bitmap
	// 18 bit: hash 
	//
	uint32_t hash;	
	// points to min node in this subtree
	//
	uint32_t minvOffset;
	// the min node's full key
	// the first indexLen bytes prefix is this node's index into the hash table
	// the first fullKeyLen bytes prefix is this node's index plus path compression part
	// the whole minKey is the min node's key
	//
	uint64_t minKey;
	// the child map
	// when using internal map, each byte stores a child
	// when using external bitmap, each bit represent whether the corresponding child exists
	// when using pointer external bitmap, this is the pointer to the 32-byte bitmap
	// when it is a leaf, this is the opaque data pointer
	// 
	uint64_t childMap;
	
	// intentionally no constructor function
	// we are always using mmap() to allocate the hash table 
	// so constructors will not be called
	//
	
	bool IsEqual(uint32_t expectedHash, int shiftLen, uint64_t shiftedKey)
	{
		return ((hash & 0xf803ffffU) == expectedHash) && (minKey >> shiftLen == shiftedKey);
	}
	
	bool IsEqualNoHash(uint64_t key, int len)
	{
		return ((hash & 0xf8000000U) == (0x80000000U | uint32_t(((len) - 1) << 27)) && (key >> (64-8*len)) == (minKey >> (64-8*len)));
	}
	
	int GetOccupyFlag()
	{
		assert((hash >> 30) != 1);
		return hash >> 30;
	}
	
	bool IsOccupied()
	{
		return GetOccupyFlag() != 0;
	}
	
	bool IsNode()
	{
		assert(IsOccupied());
		return GetOccupyFlag() == 2;
	}
	
	// the assert-less version
	//
	bool IsOccupiedAndNode()
	{
		return GetOccupyFlag() == 2;
	}
	
	uint32_t GetHash18bit()
	{
		assert(IsNode());
		return hash & ((1 << 18) - 1);
	}
	
	int GetIndexKeyLen()
	{
		assert(IsNode());
		return 1 + ((hash >> 27) & 7);
	}
	
	uint64_t GetIndexKey()
	{
		assert(IsNode());
		int shiftLen = 64 - GetIndexKeyLen() * 8;
		return minKey >> shiftLen << shiftLen;
	}
	
	// DANGER: make sure you know what you are doing...
	//
	void AlterIndexKeyLen(int newIndexKeyLen)
	{
		assert(IsNode());
		hash &= 0xc7ffffffU;
		hash |= (newIndexKeyLen - 1) << 27;
	}
	
	// DANGER: make sure you know what you are doing...
	//
	void AlterHash18bit(uint32_t hash18bit)
	{
		assert(IsNode());
		assert(0 <= hash18bit && hash18bit < (1<<18));
		hash &= 0xfffc0000;
		hash |= hash18bit;
	}
	
	int GetFullKeyLen()
	{
		assert(IsNode());
		return 1 + ((hash >> 24) & 7);
	}
	
	uint64_t GetFullKey()
	{
		assert(IsNode());
		return minKey;
	}
	
	bool IsLeaf()
	{
		assert(IsNode());
		return GetFullKeyLen() == 8;
	}
	
	bool IsUsingInternalChildMap()
	{
		assert(IsNode());
		return ((hash >> 21) & 7) == 0;
	}
	
	bool IsExternalPointerBitMap()
	{
		assert(IsNode() && !IsUsingInternalChildMap());
		return ((hash >> 21) & 7) == 4;
	}
	
	int GetChildNum()
	{
		assert(IsUsingInternalChildMap());
		return 1 + ((hash >> 18) & 7);
	}
	
	void SetChildNum(int k)
	{
		assert(IsUsingInternalChildMap());
		assert(1 <= k && k <= 8);
		hash &= 0xffe3ffffU;
		hash |= ((k-1) << 18); 
	}
	
	void Init(int ilen, int dlen, uint64_t dkey, uint32_t hash18bit, int firstChild);
	
	int FindNeighboringEmptySlot();
	
	void BitMapSet(int child);
	
	// TODO: free external bitmap memory when hash table is destroyed
	//
	uint64_t* AllocateExternalBitMap();
	
	// Switch from internal child list to internal/external bitmap
	//
	void ExtendToBitMap();
	
	// Find minimum child >= given child
	// returns -1 if larger child does not exist
	//
	int LowerBoundChild(uint32_t child);
	
	// Check if given child exists
	//
	bool ExistChild(int child);
	
	// Add a new child, must not exist
	//
	void AddChild(int child);

	// for debug only, get list of all children in sorted order
	//
	vector<int> GetAllChildren();
	
	// Copy its internal bitmap to external
	//
	uint64_t* CopyToExternalBitMap();
	
	// Move this node as well as its bitmap to target
	//
	void MoveNode(CuckooHashTableNode* target);
	
	// Relocate its internal bitmap to another position
	//
	void RelocateBitMap();
};

static_assert(sizeof(CuckooHashTableNode) == 24, "size of node should be 24");

// This class does not own the main hash table's memory
// TODO: it should manage the external bitmap memory, but not implemented yet
//
class CuckooHashTable
{
public:
#ifdef ENABLE_STATS
	struct Stats
	{
		uint32_t m_slowpathCount;
		uint32_t m_movedNodesCount;
		uint32_t m_relocatedBitmapsCount;
		uint32_t m_lcpResultHistogram[9];
		Stats();
		void ClearStats();
		void ReportStats();
	};
#endif

	class LookupMustExistPromise
	{
	public:
		LookupMustExistPromise() : valid(0) {}
		LookupMustExistPromise(CuckooHashTableNode* h)
			: valid(1)
			, h1(h)
			, h2(nullptr)
		{ }
		
		LookupMustExistPromise(uint16_t valid, uint16_t shiftLen, 
		                       CuckooHashTableNode* h1, CuckooHashTableNode* h2, 
		                       uint32_t expectedHash, uint64_t shiftedKey)
			: valid(valid)
			, shiftLen(shiftLen)
			, h1(h1)
			, h2(h2)
			, expectedHash(expectedHash)
			, shiftedKey(shiftedKey)
		{ }
		
		bool IsValid() { return valid; }
		
		uint64_t Resolve()
		{
			assert(IsValid());
			if (h2 == nullptr || h1->IsEqual(expectedHash, shiftLen, shiftedKey))
			{
				return h1->minKey;
			}
			else
			{
				assert(h1->IsEqual(expectedHash, shiftLen, shiftedKey));
				return h2->minKey;
			}
		}
		
		void Prefetch()
		{
			assert(IsValid());
			if (h2 != nullptr)
			{
				MEM_PREFETCH(*h1);
				MEM_PREFETCH(*h2);
			}
		}
		
	private:
		uint16_t valid;
		uint16_t shiftLen;
		CuckooHashTableNode* h1;
		CuckooHashTableNode* h2;
		uint32_t expectedHash;
		uint64_t shiftedKey;
	};
	
	CuckooHashTable();
	
	void Init(CuckooHashTableNode* _ht, uint64_t _mask);
	
	// Execute Cuckoo displacements to make up a slot for the specified key
	//
	uint32_t ReservePositionForInsert(int ilen, uint64_t dkey, uint32_t hash18bit, bool& exist, bool& failed);
	
	// Insert a node into the hash table
	// Since we use path-compression, if the node is not a leaf, it must has at least one child already known
	// In case it is a leaf, firstChild should be -1
	//
	uint32_t Insert(int ilen, int dlen, uint64_t dkey, int firstChild, bool& exist, bool& failed);

	// Single point lookup, returns index in hash table if found
	//
	uint32_t Lookup(int ilen, uint64_t ikey, bool& found);

	// Single point lookup on a key that is supposed to exist
	//
	CuckooHashTable::LookupMustExistPromise GetLookupMustExistPromise(int ilen, uint64_t ikey);
	
	// Fast LCP query using vectorized hash computation and memory level parallelism
	// Since we only store nodes of depth >= 3 in hash table, 
	// this function will return 2 if the LCP is < 3 (even if the real LCP is < 2).
	// In case this function returns > 2,
	//   idxLen will be the index len of the lcp node in hash table (so allPositions1[idxLen - 1] will be the lcp node)
	//   for i >= idxLen - 1, allPositions1[i] will be the node for prefix i+1 (0 if not exist)
	//   for 2 <= i < idxLen - 1, allPositions1[i] and allPositions2[i] will be the possible 2 places where the node show up,
	//   and expectedHash[i] will be its expected hash value.
	// allPositions1, allPositions2, expectedHash must be buffers at least 32 bytes long. 
	//
	int QueryLCP(uint64_t key, 
                 uint32_t& idxLen, 
                 uint32_t* allPositions1, 
                 uint32_t* allPositions2, 
                 uint32_t* expectedHash);
	
	// hash table array pointer
	//
	CuckooHashTableNode* ht;
	// hash table mask (always a power of 2 minus 1)
	//
	uint32_t htMask;
#ifdef ENABLE_STATS
	// statistic info
	//
	Stats stats;
#endif

private:
	void HashTableCuckooDisplacement(uint32_t victimPosition, int rounds, bool& failed);
	
#ifndef NDEBUG
	bool m_hasCalledInit;
#endif
};

class MlpSet
{
public:
#ifdef ENABLE_STATS
	struct Stats
	{
		uint32_t m_lowerBoundParentPathStepsHistogram[8];
		Stats();
		void ClearStats();
		void ReportStats();
	};
	Stats stats;
#endif

	typedef CuckooHashTable::LookupMustExistPromise Promise;
	
	MlpSet();
	~MlpSet();
	
	// Initialize the set to hold at most maxSetSize elements
	//
	void Init(uint32_t maxSetSize);
	
	// Insert an element, returns true if the insertion took place, false if the element already exists
	//
	bool Insert(uint64_t value);
	
	// Returns whether the specified value exists in the set
	//
	bool Exist(uint64_t value);
	
	// Returns the minimum value greater or equal to the specified value
	// set `found` to false and return -1 if specified value is larger than all values in set 
	//
	uint64_t LowerBound(uint64_t value, bool& found);
	
	// Returns a promise for lower_bound
	// Promise.IsValid() denotes if lower_bound doesn't exist
	// The promise can be resolved via Promise.Resolve() to get the lower_bound
	//
	MlpSet::Promise LowerBound(uint64_t value);
	
	// For debug purposes only
	//
	uint64_t* GetRootPtr() { return m_root; }
	uint64_t* GetLv1Ptr() { return m_treeDepth1; }
	uint64_t* GetLv2Ptr() { return m_treeDepth2; }
	CuckooHashTable* GetHtPtr() { return &m_hashTable; }
	
#ifdef ENABLE_STATS
	void ClearStats();
	void ReportStats();
#endif

private:
	MlpSet::Promise LowerBoundInternal(uint64_t value, bool& found);
	
	// we mmap memory all at once, hold the pointer to the memory chunk
	// TODO: this needs to changed after we support hash table resizing 
	//
	void* m_memoryPtr;
	uint64_t m_allocatedSize;
	
	// flat bitmap mapping parts of the tree
	// root and depth 1 should be in L1 or L2 cache
	// root of the tree, length 256 bits (32B)
	//
	uint64_t* m_root;
	// lv1 of the tree, 256^2 bits (8KB)
	//
	uint64_t* m_treeDepth1;
	// lv2 of the tree, 256^3 bits (2MB), not supposed to be in cache
	//
	uint64_t* m_treeDepth2;
	// hash mapping parts of the tree, starting at lv3
	//
	CuckooHashTable m_hashTable;
	
#ifndef NDEBUG
	bool m_hasCalledInit;
#endif
};

}	// namespace MlpSetUInt64
 
