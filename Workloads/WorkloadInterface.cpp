#include "common.h"
#include "WorkloadInterface.h"

WorkloadUInt64::WorkloadUInt64() 
	: numInitialValues(0)
	, numOperations(0)
	, initialValues(nullptr)
	, operations(nullptr)
	, results(nullptr)
	, expectedResults(nullptr)
{ }
	
void WorkloadUInt64::AllocateMemory(uint64_t _numInitialValues, uint64_t _numOperations)
{
	ReleaseAssert(initialValues == nullptr && operations == nullptr && expectedResults == nullptr);
	numInitialValues = _numInitialValues;
	numOperations = _numOperations;
	initialValues = new uint64_t[numInitialValues];
	ReleaseAssert(initialValues != nullptr);
	operations = new WorkloadOperationUInt64[numOperations];
	ReleaseAssert(operations != nullptr);
	results = new uint64_t[numOperations];
	ReleaseAssert(results != nullptr);
	memset(results, 0, sizeof(uint64_t) * numOperations);
	expectedResults = new uint64_t[numOperations];
	ReleaseAssert(expectedResults != nullptr);
	memset(expectedResults, 0, sizeof(uint64_t) * numOperations);
}
	
void WorkloadUInt64::FreeMemory()
{
	numInitialValues = 0;
	numOperations = 0;
	if (initialValues) 
	{
		delete [] initialValues;
		initialValues = nullptr;
	}
	if (operations)
	{
		delete [] operations;
		operations = nullptr;
	}
	if (results)
	{
		delete [] results;
		results = nullptr;
	}
	if (expectedResults)
	{
		delete [] expectedResults;
		expectedResults = nullptr;
	}
}

void WorkloadUInt64::PopulateExpectedResultsUsingStdSet()
{
	printf("Populating expected results using std::set..\n");
	double timePhase1, timePhase2;
	printf("Populating initial data set..\n");
	set<uint64_t> S;
	{
		AutoTimer timer(&timePhase1);
		rep(i, 0, numInitialValues - 1)
		{
			S.insert(initialValues[i]);
		}
	}
	printf("Executing operations..\n");
	{
		AutoTimer timer(&timePhase2);
		rep(i, 0, numOperations - 1)
		{
			switch (operations[i].type) 
			{
				case WorkloadOperationType::INSERT:
				{
					expectedResults[i] = S.insert(operations[i].key).second;
					break;
				}
				case WorkloadOperationType::EXIST:
				{
					expectedResults[i] = S.count(operations[i].key);
					break;
				}
				case WorkloadOperationType::LOWER_BOUND:
				{
					set<uint64_t>::iterator it = S.lower_bound(operations[i].key);
					if (it == S.end())
					{
						expectedResults[i] = 0xffffffffffffffffULL;
					}
					else
					{
						expectedResults[i] = *it;
					}
					break;
				}
				default:
				{
					ReleaseAssert(false);
				}
			}
		}
	}
	printf("Complete. Total time = %.6lf\n", timePhase1 + timePhase2);
}

void WorkloadUInt64::EnforceDependency()
{
	printf("Enforcing dependency between queries..\n");
	rep(i, 1, numOperations - 1)
	{
		uint32_t x = operations[i].type;
		x ^= (uint32_t)expectedResults[i-1];
		operations[i].type = (WorkloadOperationType)x;
		operations[i].key ^= expectedResults[i-1];
	}
	printf("Completed enforcing dependency.\n");
}

