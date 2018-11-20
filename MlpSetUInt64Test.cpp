#include "common.h"
#include "MlpSetUInt64.h"
#include "StupidTrie/Stupid64bitIntegerTrie.h"
#include "gtest/gtest.h"

namespace {

// Test of correctness of CuckooHash related logic
// This is a vitro test (not acutally a trie tree, just random data)
//
TEST(MlpSetUInt64, VitroCuckooHashLogicCorrectness)
{
	const int HtSize = 1 << 26;
	uint64_t allocatedArrLen = uint64_t(HtSize + 20) * sizeof(MlpSetUInt64::CuckooHashTableNode) + 256;
	void* allocatedPtr = mmap(NULL, 
		                      allocatedArrLen, 
		                      PROT_READ | PROT_WRITE, 
		                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, 
		                      -1 /*fd*/, 
		                      0 /*offset*/);
	ReleaseAssert(allocatedPtr != MAP_FAILED);
	Auto(
		int result = SAFE_HUGETLB_MUNMAP(allocatedPtr, allocatedArrLen);
		ReleaseAssert(result == 0);
	);
	
	memset(allocatedPtr, 0, allocatedArrLen);
	
	MlpSetUInt64::CuckooHashTable ht;
	
	{
		uintptr_t x = reinterpret_cast<uintptr_t>(allocatedPtr);
		x += 6 * sizeof(MlpSetUInt64::CuckooHashTableNode);
		x = x / 128 * 128;
		ht.Init(reinterpret_cast<MlpSetUInt64::CuckooHashTableNode*>(x), HtSize - 1);
	}
	
	vector<StupidUInt64Trie::TrieNodeDescriptor> data;
	
	// gen data
	//
	{
		printf("Generating data..\n");
		int remainingNodes = 0.45 * HtSize;	// 45% load factor
		int numNodesForTenPercent = remainingNodes / 10;
		data.reserve(remainingNodes);
		set<uint64_t> S[9];
		while (remainingNodes > 0)
		{
			int ilen;
			uint64_t key;
			// generate a node
			//
			while (1)
			{
				ilen = rand() % 6 + 3;
				key = 0;
				rep(i, 1, ilen)
				{
					key = key * 256 + rand() % 256;
				}
				if (S[ilen].count(key) == 0)
				{
					break;
				}
			}
			S[ilen].insert(key);
			int dlen = rand() % (9 - ilen) + ilen;
			remainingNodes--;
			
			rep(i, ilen+1, 8)
			{
				key = key * 256 + rand() % 256;
			}
			
			// generate some childs
			//
			int numChilds = (rand() % 3 == 0) ? (8 + rand() % 20) : (rand() % 8 + 1);
			if (dlen == 8)
			{
				numChilds = 0;
			}
			bool exist[256]; 
			memset(exist, 0, 256);
			rep(i, 1, numChilds) 
			{
				exist[rand() % 256] = 1;
			}
			int actualNumChilds = 0;
			rep(i, 0, 255)
			{
				if (exist[i])
				{
					actualNumChilds++;
				}
			}
			if (actualNumChilds > 8) 
			{
				remainingNodes--;
			}
			
			data.push_back(StupidUInt64Trie::TrieNodeDescriptor(ilen, dlen, key, actualNumChilds));
			rep(i, 0, 255)
			{
				if (exist[i])
				{
					data.back().AddChild(i);
				}
			}
			if (remainingNodes % numNodesForTenPercent == 0)
			{
				printf("%d%% complete\n", 100 - remainingNodes / numNodesForTenPercent * 10);
			}
		}
		printf("Data generation complete. %d records generated\n", int(data.size()));
	}
	
	// insert data, check each row is as expected right after its insertion
	//
	printf("Inserting data..\n");
	rept(row, data)
	{
		int childCount = row->children.size();
		int firstChild = -1;
		if (childCount > 0)
		{
			firstChild = row->children[0];
		}
		bool exist, failed;
		uint32_t pos = ht.Insert(row->ilen, row->dlen, row->minv, firstChild, exist, failed);
		ReleaseAssert(!exist);
		ReleaseAssert(!failed);
		ReleaseAssert(ht.ht[pos].GetIndexKeyLen() == row->ilen);
		ReleaseAssert(ht.ht[pos].GetFullKeyLen() == row->dlen);
		ReleaseAssert(ht.ht[pos].GetFullKey() == row->minv);
		rep(i, 1, childCount - 1)
		{
			ht.ht[pos].AddChild(row->children[i]);
		}
		if (childCount > 0)
		{
			vector<int> ch = ht.ht[pos].GetAllChildren();
			ReleaseAssert(ch.size() == row->children.size());
			rep(i, 0, int(ch.size()) - 1)
			{
				ReleaseAssert(ch[i] == row->children[i]);
			}
		}
	}
	printf("Insertion complete.\n");
	printf("Cuckoo hash table stats: %u node moves %u bitmap moves\n", 
		   ht.stats.m_movedNodesCount, ht.stats.m_relocatedBitmapsCount);
		       
	// sanity check whole hash table
	//
	{
		printf("Sanity check hash table..\n");
		int nodeCount = 0;
		int bitmapCount = 0;
		int bitmapCount2 = 0;
		int extMapCount = 0;
		rep(i, -3, HtSize + 2)
		{
			if (ht.ht[i].IsOccupied())
			{
				if (ht.ht[i].IsNode())
				{
					nodeCount++;
					if (!ht.ht[i].IsUsingInternalChildMap())
					{
						bitmapCount++;
						if (ht.ht[i].IsExternalPointerBitMap())
						{
							extMapCount++;
						}
						else
						{
							int offset = (ht.ht[i].hash >> 21) & 7;
							assert(ht.ht[i+offset-4].IsOccupied() && !ht.ht[i+offset-4].IsNode());
						}
					}
				}
				else
				{
					bitmapCount2++;
				}
			}
		}
		ReleaseAssert(bitmapCount == bitmapCount2 + extMapCount);
		ReleaseAssert(nodeCount == data.size());
		printf("Hash table sanity check complete.\n");
		printf("Hash table bitmap stats: %d bitmaps %d external bitmaps\n", bitmapCount, extMapCount);
	}
	
	// Lookup for each row of data again and make sure everything is as expected
	//
	printf("Performing lookup for all inserted rows..\n");
	rept(row, data)
	{
		bool found;
		uint32_t pos = ht.Lookup(row->ilen, row->minv, found);
		ReleaseAssert(found);
		ReleaseAssert(ht.ht[pos].GetIndexKeyLen() == row->ilen);
		ReleaseAssert(ht.ht[pos].GetFullKeyLen() == row->dlen);
		ReleaseAssert(ht.ht[pos].GetFullKey() == row->minv);
		int childCount = row->children.size();
		if (childCount > 0)
		{
			vector<int> ch = ht.ht[pos].GetAllChildren();
			ReleaseAssert(ch.size() == row->children.size());
			rep(i, 0, int(ch.size()) - 1)
			{
				ReleaseAssert(ch[i] == row->children[i]);
			}
		}
		else
		{
			ReleaseAssert(ht.ht[pos].IsLeaf());
		}
	}
	printf("Hash table lookup check complete.\n");
}

// Test of correctness of CuckooHash.QueryLCP() function
// This is a vitro test (not acutally a trie tree, just random data)
//
TEST(MlpSetUInt64, VitroCuckooHashQueryLcpCorrectness)
{
	const int HtSize = 1 << 26;
	uint64_t allocatedArrLen = uint64_t(HtSize + 20) * sizeof(MlpSetUInt64::CuckooHashTableNode) + 256;
	void* allocatedPtr = mmap(NULL, 
		                      allocatedArrLen, 
		                      PROT_READ | PROT_WRITE, 
		                      MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, 
		                      -1 /*fd*/, 
		                      0 /*offset*/);
	ReleaseAssert(allocatedPtr != MAP_FAILED);
	Auto(
		int result = SAFE_HUGETLB_MUNMAP(allocatedPtr, allocatedArrLen);
		ReleaseAssert(result == 0);
	);
	
	memset(allocatedPtr, 0, allocatedArrLen);
	
	MlpSetUInt64::CuckooHashTable ht;
	
	{
		uintptr_t x = reinterpret_cast<uintptr_t>(allocatedPtr);
		x += 6 * sizeof(MlpSetUInt64::CuckooHashTableNode);
		x = x / 128 * 128;
		ht.Init(reinterpret_cast<MlpSetUInt64::CuckooHashTableNode*>(x), HtSize - 1);
	}
	
	const int numQueries = 20000000;
	uint64_t* q = new uint64_t[numQueries];
	pair<int, uint64_t>* expectedAnswers = new pair<int, uint64_t>[numQueries];
	pair<int, uint64_t>* actualAnswers = new pair<int, uint64_t>[numQueries];
	ReleaseAssert(q != nullptr);
	ReleaseAssert(expectedAnswers != nullptr);
	ReleaseAssert(actualAnswers != nullptr);
	Auto(delete [] q);
	Auto(delete [] expectedAnswers);
	Auto(delete [] actualAnswers);
	
	rep(i,0,numQueries-1)
	{
		actualAnswers[i].first = 0;
		actualAnswers[i].second = 0;
	}
	
	{
		printf("Generating data..\n");
		int totalNodes = 0.45 * HtSize;	// 45% load factor
		map<uint64_t, uint64_t> S[9];
		rep(currentNode, 0, totalNodes - 1)
		{
			int ilen;
			uint64_t key;
			// generate a node
			//
			while (1)
			{
				ilen = rand() % 6 + 3;
				key = 103;
				rep(i, 2, ilen)
				{
					key = key * 256 + rand() % 20;
				}
				if (S[ilen].count(key) == 0)
				{
					break;
				}
			}
			int dlen = rand() % (9 - ilen) + ilen;

			uint64_t fullKey = key;
			rep(i, ilen+1, 8)
			{
				fullKey = fullKey * 256 + rand() % 20;
			}
			
			S[ilen][key] = fullKey;
			
			bool exist, failed;
			uint32_t pos = ht.Insert(ilen, dlen, fullKey, (dlen == 8 ? -1 : 233) /*firstChild*/, exist, failed);
			ReleaseAssert(!exist);
			ReleaseAssert(!failed);
			ReleaseAssert(ht.ht[pos].GetIndexKeyLen() == ilen);
			ReleaseAssert(ht.ht[pos].GetFullKeyLen() == dlen);
			ReleaseAssert(ht.ht[pos].GetFullKey() == fullKey);
			
			if (currentNode % (totalNodes / 10) == 0)
			{
				printf("%d%% complete\n", currentNode / (totalNodes / 10) * 10);
			}
		}
		printf("Generating query..\n");
		// generate query
		//
		rep(i,0,numQueries-1)
		{
			uint64_t key = 103;
			rep(k,2,8)
			{
				key = key * 256 + rand() % 20;
			}
			if (rand() % 10 == 0)
			{
				key = 103 * 256 + rand() % 20;
				rep(k,3,8)
				{
					key = key * 256 + rand() % 256;
				}
			}
			q[i] = key;
			expectedAnswers[i] = make_pair(2, uint64_t(-1));
			repd(len, 8, 3)
			{
				if (S[len].count(q[i] >> (64 - len*8)))
				{	
					uint64_t v = S[len][q[i] >> (64 - len*8)];
					expectedAnswers[i].second = v;
					uint64_t xorValue = q[i] ^ v;
					if (!xorValue) 
					{	
						expectedAnswers[i].first = 8;
					}
					else
					{
						int z = __builtin_clzll(xorValue);
						expectedAnswers[i].first = z / 8;
					}
					break;
				}
			}
			if (i % (numQueries / 10) == 0)
			{
				printf("%d%% complete\n", i / (numQueries / 10) * 10);
			}
		}
		printf("Data generation complete. %d records generated, %d query generated\n", totalNodes, numQueries);
	}
	
	printf("Executing queries..\n");
	{
		AutoTimer timer;
		rep(i,0,numQueries - 1)
		{
			uint32_t pos;
			int len = ht.QueryLCP(q[i], pos /*out*/);
			actualAnswers[i].first = len;
			if (len == 2)
			{
				actualAnswers[i].second = uint64_t(-1);
			}
			else
			{
				actualAnswers[i].second = ht.ht[pos].minKey;
			}
		}
	}
	
	printf("Query completed.\n");
	printf("Hash table stats: %u slowpath count\n", ht.stats.m_slowpathCount);
	
	printf("Validating answers..\n");
	{
		int cnt[10];
		memset(cnt, 0, sizeof(int) * 10);
		rep(i,0,numQueries - 1)
		{
			ReleaseAssert(actualAnswers[i] == expectedAnswers[i]);
			cnt[actualAnswers[i].first]++;
		}
		printf("Test complete.\n");
		printf("Query result stats:\n");
		rep(i,2,8)
		{
			printf("LCP = %d: %d\n", i, cnt[i]);
		}
	}
}

}	// annoymous namespace


