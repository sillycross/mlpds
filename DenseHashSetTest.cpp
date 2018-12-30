#include "gtest/gtest.h"

#include "dense_hash_set_wrapper.h"
#include "common.h"
#include "WorkloadA.h"
#include "WorkloadC.h"

TEST(DenseHashSetUInt64, WorkloadA_80M_Dep)
{
	printf("Generating workload WorkloadA 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadA::GenWorkload80M();
	Auto(workload.FreeMemory());
	
	rep(i, 0, workload.numOperations - 1)
	{
		workload.expectedResults[i] *= workload.operations[i].key;
	}
	
	workload.EnforceDependency();
	
	repd(i, workload.numInitialValues-1, 1)
	{
		workload.initialValues[i] ^= workload.initialValues[i-1];
	}
	
	printf("Executing workload..\n");
	DenseHashSetUInt64::DenseHashSetExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += (workload.results[i] > 0);
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(DenseHashSetUInt64, WorkloadA_80M_NoDep)
{
	printf("Generating workload WorkloadA 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadA::GenWorkload80M();
	Auto(workload.FreeMemory());
	
	printf("Executing workload..\n");
	DenseHashSetUInt64::DenseHashSetExecuteWorkload<false>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += workload.results[i];
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

TEST(DenseHashSetUInt64, WorkloadC_80M_Dep)
{
	printf("Generating workload WorkloadC 80M ENFORCE dep..\n");
	WorkloadUInt64 workload = WorkloadC::GenWorkload80M();
	Auto(workload.FreeMemory());
	
	rep(i, 0, workload.numOperations - 1)
	{
		workload.expectedResults[i] *= workload.operations[i].key;
	}
	
	workload.EnforceDependency();
	
	repd(i, workload.numInitialValues-1, 1)
	{
		workload.initialValues[i] ^= workload.initialValues[i-1];
	}
	
	printf("Executing workload..\n");
	DenseHashSetUInt64::DenseHashSetExecuteWorkload<true>(workload);
	
	printf("Validating results..\n");
	uint64_t sum = 0;
	rep(i, 0, workload.numOperations - 1)
	{
		ReleaseAssert(workload.results[i] == workload.expectedResults[i]);
		sum += (workload.results[i] > 0);
	}
	printf("Finished %d queries %d positives\n", int(workload.numOperations), int(sum));
}

