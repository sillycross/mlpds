#include "common.h"
#include "WorkloadInterface.h"
#include "WorkloadA.h"
#include "WorkloadB.h"
#include "WorkloadC.h"
#include "WorkloadD.h"
#include "hot_wrapper.h"
#include "gtest/gtest.h"

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

}	// namespace


