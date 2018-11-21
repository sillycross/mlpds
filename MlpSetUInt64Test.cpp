#include "common.h"
#include "MlpSetUInt64.h"
#include "StupidTrie/Stupid64bitIntegerTrie.h"
#include "WorkloadInterface.h"
#include "WorkloadA.h"
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
			uint64_t buffer[4];
			int len = ht.QueryLCP(q[i], pos /*out*/, reinterpret_cast<uint32_t*>(buffer));
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

// An incomprehensive assert
// It checks that every node in StupidTrie exists and has the same information in MlpSet
// but it does not check the other direction
//
void AssertTreeShapeEqualA(StupidUInt64Trie::Trie& st, MlpSetUInt64::MlpSet& ms, bool printDetail)
{
	vector<StupidUInt64Trie::TrieNodeDescriptor> nodeList;
	st.DumpData(nodeList);
	rept(it, nodeList)
	{
		int ilen = it->ilen;
		int dlen = it->dlen;
		uint64_t key = it->minv;
		ReleaseAssert((ms.GetRootPtr()[(key >> 56) / 64] & (uint64_t(1) << ((key >> 56) % 64))) != 0);
		ReleaseAssert((ms.GetLv1Ptr()[(key >> 48) / 64] & (uint64_t(1) << ((key >> 48) % 64))) != 0);
		ReleaseAssert((ms.GetLv2Ptr()[(key >> 40) / 64] & (uint64_t(1) << ((key >> 40) % 64))) != 0);
		if (ilen >= 4)
		{
			bool found;
			uint32_t pos = ms.GetHtPtr()->Lookup(3, key, found);
			ReleaseAssert(found);
		}
		if (ilen >= 3)
		{
			bool found;
			uint32_t pos = ms.GetHtPtr()->Lookup(ilen, key, found);
			ReleaseAssert(found);
			ReleaseAssert(ms.GetHtPtr()->ht[pos].GetIndexKeyLen() == ilen);
			ReleaseAssert(ms.GetHtPtr()->ht[pos].GetFullKeyLen() == dlen);
			ReleaseAssert(ms.GetHtPtr()->ht[pos].minKey == key);
			vector<int> ch = ms.GetHtPtr()->ht[pos].GetAllChildren();
			ReleaseAssert(ch.size() == it->children.size());
			rep(i, 0, int(ch.size()) - 1)
			{
				ReleaseAssert(ch[i] == it->children[i]);
			}
		}
	}
	if (printDetail)
	{
		printf("%d nodes in trie tree\n", int(nodeList.size()));
	}
}

// A correctness test for MlpSet.Insert()
// Inserts a bunch of elements, verify the whole trie shape is as expected after each insertion
//
TEST(MlpSetUInt64, MlpSetInsertStepByStepCorrectness)
{
	const int N = 14000;
	vector< vector<int> > choices;
	choices.resize(8);
	rep(i,0,7)
	{
		int sz = (i <= 1) ? 2 : 5;
		rep(j,0,sz-1)
		{
			int x = rand() % 256;
			choices[i].push_back(x);
		}
	}
	StupidUInt64Trie::Trie st;
	MlpSetUInt64::MlpSet ms;
	ms.Init(N + 1000);
	rep(steps, 0, N-1)
	{
		uint64_t value = 0;
		rep(i, 0, 7)
		{
			value = value * 256 + choices[i][rand() % choices[i].size()];
		}
		bool st_inserted = st.Insert(value);
		bool ms_inserted = ms.Insert(value);
		ReleaseAssert(st_inserted == ms_inserted);
		AssertTreeShapeEqualA(st, ms, (steps % (N / 10) == 0) /*printDetail*/);
		if (steps % (N / 10) == 0)
		{
			printf("%d%% completed\n", steps / (N / 10) * 10);
		}
	}
}

// A larger correctness test for MlpSet.Insert()
// Inserts a bunch of elements, verify the whole trie shape is as expected in the end
//
TEST(MlpSetUInt64, MlpSetInsertCorrectness)
{
	const int N = 16000000;
	vector< vector<int> > choices;
	choices.resize(8);
	rep(i,0,7)
	{
		int sz = (i <= 1) ? 6 : 12;
		set<int> s;
		rep(j,0,sz-1)
		{
			int x;
			while (1)
			{
				x = rand() % 256;
				if (!s.count(x)) break;
			}
			s.insert(x);
			choices[i].push_back(x);
		}
	}
	
	uint64_t* values = new uint64_t[N];
	ReleaseAssert(values != nullptr);
	Auto(delete [] values);
	
	printf("Generating data N = %d..\n", N);
	rep(k,0,N-1) 
	{
		uint64_t x = 0;
		rep(i, 0, 7)
		{
			x = x * 256 + choices[i][rand() % choices[i].size()];
		}
		values[k] = x;
	}
	
	StupidUInt64Trie::Trie st;
	
	printf("Stupid trie insertion..\n");
	int numDistinct = 0;
	rep(i,0,N-1)
	{
		bool x = st.Insert(values[i]);
		if (x) numDistinct++;
		if (i % (N / 10) == 0)
		{
			printf("%d%% complete\n", i / (N / 10) * 10);
		}
	}
	printf("Insertion complete. %d distinct elements\n", numDistinct);
	
	MlpSetUInt64::MlpSet ms;
	ms.Init(N + 1000);
	
	printf("MlpSet insertion..\n");
	{
		AutoTimer timer;
		int numDistinctMlpTrie = 0;
		rep(i,0,N-1)
		{
			bool x = ms.Insert(values[i]);
			if (x) numDistinctMlpTrie++;
		}
		ReleaseAssert(numDistinct == numDistinctMlpTrie);
	}
	
	printf("MlpSet insertion complete. Validating..\n");
	AssertTreeShapeEqualA(st, ms, true /*printDetail*/);
	printf("Test complete.\n");
	printf("Hash table stats: %u slowpath, %u node moves, %u bitmap relocation\n", 
	       ms.GetHtPtr()->stats.m_slowpathCount, 
	       ms.GetHtPtr()->stats.m_movedNodesCount, 
	       ms.GetHtPtr()->stats.m_relocatedBitmapsCount);
}

template<bool enforcedDep>
void NO_INLINE MlpSetExecuteWorkload(WorkloadUInt64& workload)
{
	printf("MlpSet executing workload, enforced dependency = %d\n", (enforcedDep ? 1 : 0));
	MlpSetUInt64::MlpSet ms;
	ms.Init(workload.numInitialValues + 1000);
	
	printf("MlpSet populating initial values..\n");
	{
		AutoTimer timer;
		rep(i, 0, workload.numInitialValues - 1)
		{
			ms.Insert(workload.initialValues[i]);
		}
	}
	
	printf("MlpSet executing workload..\n");
	{
		AutoTimer timer;
		if (enforcedDep)
		{
			uint64_t lastAnswer = 0;
			rep(i, 0, workload.numOperations - 1)
			{
				uint32_t x = workload.operations[i].type;
				x ^= (uint32_t)lastAnswer;
				WorkloadOperationType type = (WorkloadOperationType)x;
				uint64_t realKey = workload.operations[i].key ^ lastAnswer;
				uint64_t answer;
				switch (type)
				{
					case WorkloadOperationType::INSERT:
					{
						answer = ms.Insert(realKey);
						break;
					}
					case WorkloadOperationType::EXIST:
					{
						answer = ms.Exist(realKey);
						break;
					}
				}
				workload.results[i] = answer;
				lastAnswer = answer;
			}
		}
		else
		{
			rep(i, 0, workload.numOperations - 1)
			{
				uint64_t answer;
				switch (workload.operations[i].type)
				{
					case WorkloadOperationType::INSERT:
					{
						answer = ms.Insert(workload.operations[i].key);
						break;
					}
					case WorkloadOperationType::EXIST:
					{
						answer = ms.Exist(workload.operations[i].key);
						break;
					}
				}
				workload.results[i] = answer;
			}
		}
	}
	
	printf("MlpSet workload completed.\n");
}

TEST(MlpSetUInt64, WorkloadA_16M)
{
	printf("Generating workload WorkloadA 16M NO-ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadA::GenWorkload16M();
	Auto(workload.FreeMemory());
	
	printf("Executing workload..\n");
	MlpSetExecuteWorkload<false>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(MlpSetUInt64, WorkloadA_16M_Dep)
{
	printf("Generating workload WorkloadA 16M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadA::GenWorkload16M();
	Auto(workload.FreeMemory());
	
	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	MlpSetExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

}	// annoymous namespace


