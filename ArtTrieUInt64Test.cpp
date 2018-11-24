#include "common.h"
#include "WorkloadInterface.h"
#include "WorkloadA.h"
#include "WorkloadC.h"
#include "third_party/libart/art.h"
#include "gtest/gtest.h"

namespace
{

template<bool enforcedDep>
void NO_INLINE ArtTrieExecuteWorkload(WorkloadUInt64& workload)
{
	// ART trie takes in string keys. 
	// So in order to achieve the same effect, we need to reverse the bytes in the key
	// Do this outside the benchmark loop so it won't negatively affect ART's result
	//
	rep(i, 0, workload.numInitialValues - 1)
	{
		uint64_t x = workload.initialValues[i];
		uint64_t y = 0;
		rep(k, 0, 7)
		{
			y = y * 256 + x % 256;
			x /= 256;
		}
		workload.initialValues[i] = y;
	}
	rep(i, 0, workload.numOperations - 1)
	{
		uint64_t x = workload.operations[i].key;
		uint64_t y = 0;
		rep(k, 0, 7)
		{
			y = y * 256 + x % 256;
			x /= 256;
		}
		workload.operations[i].key = y;
	}
	
	// We have to enforce dependency after the above pre-processing
	//
	if (enforcedDep)
	{
		workload.EnforceDependency();
	}
	
	printf("ART trie executing workload, enforced dependency = %d\n", (enforcedDep ? 1 : 0));
	art_tree t;
	{
		int ret = art_tree_init(&t);
		ReleaseAssert(ret == 0);
	}
	
	printf("ART trie populating initial values..\n");
	{
		AutoTimer timer;
		rep(i, 0, workload.numInitialValues - 1)
		{
			art_insert(&t, reinterpret_cast<const unsigned char *>(&(workload.initialValues[i])), 8, reinterpret_cast<void*>(1));
		}
	}
	
	printf("ART trie executing workload..\n");
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
						answer = reinterpret_cast<uint64_t>(
						             art_insert(&t,
						                        reinterpret_cast<const unsigned char *>(&realKey),
						                        8,
						                        reinterpret_cast<void*>(1)));
						break;
					}
					case WorkloadOperationType::EXIST:
					{
						answer = reinterpret_cast<uint64_t>(
						             art_search(&t,
						                        reinterpret_cast<const unsigned char *>(&realKey),
						                        8));
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
						answer = reinterpret_cast<uint64_t>(
						             art_insert(&t,
						                        reinterpret_cast<const unsigned char *>(&(workload.operations[i].key)),
						                        8,
						                        reinterpret_cast<void*>(1)));
						break;
					}
					case WorkloadOperationType::EXIST:
					{
						answer = reinterpret_cast<uint64_t>(
						             art_search(&t,
						                        reinterpret_cast<const unsigned char *>(&(workload.operations[i].key)),
						                        8));
						break;
					}
				}
				workload.results[i] = answer;
			}
		}
	}
	
	{
		int ret = art_tree_destroy(&t);
		ReleaseAssert(ret == 0);
	}
	
	printf("ART trie workload completed.\n");
}

TEST(ArtTrieUInt64, WorkloadA_16M)
{
	printf("Generating workload WorkloadA 16M NO-ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadA::GenWorkload16M();
	Auto(workload.FreeMemory());
	
	printf("Executing workload..\n");
	ArtTrieExecuteWorkload<false>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(ArtTrieUInt64, WorkloadA_16M_Dep)
{
	printf("Generating workload WorkloadA 16M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadA::GenWorkload16M();
	Auto(workload.FreeMemory());
	
	printf("Executing workload..\n");
	ArtTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(ArtTrieUInt64, WorkloadA_80M_Dep)
{
	printf("Generating workload WorkloadA 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadA::GenWorkload80M();
	Auto(workload.FreeMemory());
	
	printf("Executing workload..\n");
	ArtTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(ArtTrieUInt64, WorkloadC_16M_Dep)
{
	printf("Generating workload WorkloadC 16M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadC::GenWorkload16M();
	Auto(workload.FreeMemory());
	
	printf("Executing workload..\n");
	ArtTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(ArtTrieUInt64, WorkloadC_80M_Dep)
{
	printf("Generating workload WorkloadC 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadC::GenWorkload80M();
	Auto(workload.FreeMemory());
	
	printf("Executing workload..\n");
	ArtTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

}	// namespace


