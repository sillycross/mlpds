#include "common.h"
#include "WorkloadInterface.h"
#include "WorkloadA.h"
#include "WorkloadB.h"
#include "WorkloadC.h"
#include "WorkloadD.h"
#include "hot_wrapper.h"
#include "gtest/gtest.h"

#include <hot/singlethreaded/HOTSingleThreaded.hpp>
#include <idx/contenthelpers/IdentityKeyExtractor.hpp>
#include <idx/contenthelpers/OptionalValue.hpp>

namespace
{

TEST(HotTrieUInt64, WorkloadA_16M)
{
	printf("Generating workload WorkloadA 16M NO-ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadA::GenWorkload16M();
	Auto(workload.FreeMemory());
	
	printf("Executing workload..\n");
	HotTrieUInt64::HotTrieExecuteWorkload<false>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(HotTrieUInt64, WorkloadA_16M_Dep)
{
	printf("Generating workload WorkloadA 16M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadA::GenWorkload16M();
	Auto(workload.FreeMemory());
	
	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	HotTrieUInt64::HotTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(HotTrieUInt64, WorkloadA_80M_Dep)
{
	printf("Generating workload WorkloadA 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadA::GenWorkload80M();
	Auto(workload.FreeMemory());
	
	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	HotTrieUInt64::HotTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(HotTrieUInt64, WorkloadB_16M_Dep)
{
	printf("Generating workload WorkloadB 16M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadB::GenWorkload16M();
	Auto(workload.FreeMemory());
	
	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	HotTrieUInt64::HotTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
	}
	printf("Finished %d queries\n", int(workload.numOperations));
}

TEST(HotTrieUInt64, WorkloadB_80M_Dep)
{
	printf("Generating workload WorkloadB 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadB::GenWorkload80M();
	Auto(workload.FreeMemory());
	
	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	HotTrieUInt64::HotTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
	}
	printf("Finished %d queries\n", int(workload.numOperations));
}

TEST(HotTrieUInt64, WorkloadC_16M_Dep)
{
	printf("Generating workload WorkloadC 16M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadC::GenWorkload16M();
	Auto(workload.FreeMemory());
	
	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	HotTrieUInt64::HotTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(HotTrieUInt64, WorkloadC_80M_Dep)
{
	printf("Generating workload WorkloadC 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadC::GenWorkload80M();
	Auto(workload.FreeMemory());
	
	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	HotTrieUInt64::HotTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(HotTrieUInt64, WorkloadD_16M_Dep)
{
	printf("Generating workload WorkloadD 16M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadD::GenWorkload16M();
	Auto(workload.FreeMemory());
	
	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	HotTrieUInt64::HotTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
	}
	printf("Finished %d queries\n", int(workload.numOperations));
}

TEST(HotTrieUInt64, WorkloadD_80M_Dep)
{
	printf("Generating workload WorkloadD 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadD::GenWorkload80M();
	Auto(workload.FreeMemory());
	
	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	HotTrieUInt64::HotTrieExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
	}
	printf("Finished %d queries\n", int(workload.numOperations));
}

typedef hot::singlethreaded::HOTSingleThreaded<uint64_t, idx::contenthelpers::IdentityKeyExtractor> HotSetUInt64;

TEST(HotTrieUInt64, Iteration)
{
	const int N = 80000000;
	const int Q = 20000000;
	WorkloadUInt64 workload;
	workload.AllocateMemory(N, Q);
	rep(i, 0, N-1)
	{
		uint64_t key = 0;
		rep(k, 2, 7)
		{
			key = key * 256 + rand() % 6 + 48;
		}
		rep(k, 0, 1)
		{
			key = key * 256 + rand() % 96 + 32;
		}
		
		workload.initialValues[i] = key;
	}
	
	HotSetUInt64 s;
	{
		AutoTimer timer;
		rep(i, 0, workload.numInitialValues - 1)
		{
			s.insert(workload.initialValues[i]);
		}
	}
	
	printf("iteration..\n");
	
	uint64_t sum = 0;
	uint64_t cnt = 0;
	{
		AutoTimer timer;
		auto it = s.begin();
		while (it != s.end()) 
		{
			sum += *it;
			cnt++;
			++it;
		}
	}
	printf("%llu %llu\n", cnt, sum);
}

}	// namespace


