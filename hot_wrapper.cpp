#include "hot_wrapper.h"

#include <algorithm>
#include <cassert>
#include <chrono>
#include <iostream>
#include <map>

#include <hot/singlethreaded/HOTSingleThreaded.hpp>
#include <idx/contenthelpers/IdentityKeyExtractor.hpp>
#include <idx/contenthelpers/OptionalValue.hpp>

typedef hot::singlethreaded::HOTSingleThreaded<uint64_t, idx::contenthelpers::IdentityKeyExtractor> HotSetUInt64;

namespace HotTrieUInt64
{

template<bool enforcedDep>
void HotTrieExecuteWorkload(WorkloadUInt64& workload)
{
	printf("HotTrie executing workload, enforced dependency = %d\n", (enforcedDep ? 1 : 0));
	
	HotSetUInt64 s;
	
	printf("HotTrie populating initial values..\n");
	{
		AutoTimer timer;
		rep(i, 0, workload.numInitialValues - 1)
		{
			s.insert(workload.initialValues[i]);
		}
	}
	
	printf("HotTrie executing workload..\n");
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
						bool r = s.insert(realKey);
						answer = r;
						break;
					}
					case WorkloadOperationType::EXIST:
					{
						idx::contenthelpers::OptionalValue<uint64_t> result = s.lookup(realKey);
						bool r = (result.mIsValid & (result.mValue == realKey));
						answer = r;
						break;
					}
					case WorkloadOperationType::LOWER_BOUND:
					{
						auto it = s.lower_bound(realKey);
						if (it == s.end()) 
						{
							answer = 0xffffffffffffffffULL;
						}
						else
						{
							answer = *it;
						}
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
				uint64_t realKey = workload.operations[i].key;
				switch (workload.operations[i].type)
				{
					case WorkloadOperationType::INSERT:
					{
						bool r = s.insert(realKey);
						answer = r;
						break;
					}
					case WorkloadOperationType::EXIST:
					{
						idx::contenthelpers::OptionalValue<uint64_t> result = s.lookup(realKey);
						bool r = (result.mIsValid & (result.mValue == realKey));
						answer = r;
						break;
					}
					case WorkloadOperationType::LOWER_BOUND:
					{
						auto it = s.lower_bound(realKey);
						if (it == s.end()) 
						{
							answer = 0xffffffffffffffffULL;
						}
						else
						{
							answer = *it;
						}
						break;
					}
				}
				workload.results[i] = answer;
			}
		}
	}
	
	printf("HotTrie workload completed.\n");
}

// explicitly instantiate template function
//
template void HotTrieExecuteWorkload<false>(WorkloadUInt64& workload);
template void HotTrieExecuteWorkload<true>(WorkloadUInt64& workload);

}	// HotTrieUInt64

