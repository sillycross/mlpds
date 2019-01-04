#include "ebs_wrapper.h"

#include "common.h"

#include "btree_array.h"

namespace EbsUInt64
{

using namespace fbs;

typedef btree_array<64/sizeof(uint64_t),uint64_t,uint64_t,true> EbsArray;

template<bool enforcedDep>
void EbsExecuteWorkload(WorkloadUInt64& workload)
{
	printf("Ebs executing workload, enforced dependency = %d\n", (enforcedDep ? 1 : 0));
	
	int n = workload.numInitialValues;
	sort(workload.initialValues, workload.initialValues + n);
	
	EbsArray A(workload.initialValues, workload.numInitialValues);

	printf("Ebs executing workload..\n");
	{
		AutoTimer timer;
		if (enforcedDep)
		{
			uint64_t lastAnswer = 0;
			for (int i = 0; i < workload.numOperations; i++)
			{
				uint32_t x = workload.operations[i].type;
				x ^= (uint32_t)lastAnswer;
				WorkloadOperationType type = (WorkloadOperationType)x;
				uint64_t realKey = workload.operations[i].key ^ lastAnswer;
				uint64_t answer;
				switch (type)
				{
					case WorkloadOperationType::EXIST:
					{
						int x = A.search(realKey);
						if (x < n)
							answer = (A.get_data(x) == realKey);
						else
							answer = false;
						break;
					}
					case WorkloadOperationType::LOWER_BOUND:
					{
						int x = A.search(realKey);
						if (x < n)
							answer = A.get_data(x);
						else
							answer = 0xffffffffffffffffULL;
						break;
					}
				}
				workload.results[i] = answer;
				lastAnswer = answer;
			}
		}
		else
		{
			ReleaseAssert(false);
		}
	}
	
	printf("DenseHashSet workload completed.\n");
}

// explicitly instantiate template function
//
template void EbsExecuteWorkload<false>(WorkloadUInt64& workload);
template void EbsExecuteWorkload<true>(WorkloadUInt64& workload);

}	// DenseHashSetUInt64

