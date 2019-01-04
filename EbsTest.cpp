#include "gtest/gtest.h"

#include "ebs_wrapper.h"
#include "common.h"
#include "WorkloadA.h"
#include "WorkloadB.h"
#include "WorkloadC.h"
#include "WorkloadD.h"

namespace {

TEST(EbsUInt64Test, WorkloadA_80M_Dep)
{
	printf("Generating workload WorkloadA 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadA::GenWorkload80M();
	Auto(workload.FreeMemory());

	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	EbsUInt64::EbsExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += (workload.results[i] > 0);
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(EbsUInt64Test, WorkloadB_80M_Dep)
{
	printf("Generating workload WorkloadB 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadB::GenWorkload80M();
	Auto(workload.FreeMemory());

	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	EbsUInt64::EbsExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += (workload.results[i] > 0);
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(EbsUInt64Test, WorkloadC_80M_Dep)
{
	printf("Generating workload WorkloadC 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadC::GenWorkload80M();
	Auto(workload.FreeMemory());

	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	EbsUInt64::EbsExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += (workload.results[i] > 0);
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(EbsUInt64Test, WorkloadD_80M_Dep)
{
	printf("Generating workload WorkloadD 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadD::GenWorkload80M();
	Auto(workload.FreeMemory());

	workload.EnforceDependency();
	
	printf("Executing workload..\n");
	EbsUInt64::EbsExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += (workload.results[i] > 0);
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}


}	// annoymous namespace
