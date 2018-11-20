#include "common.h"
#include "gtest/gtest.h"

namespace {

struct atype
{
	uint64_t value[8];
};

static_assert(sizeof(atype) == 64, "size of atype should be 64");

const int memMB = 1024;		// must be power of 2
const int numQueries = 100000000;

atype* AllocateMemory(bool usingHugePage)
{
	uint64_t numBytes = memMB * uint64_t(1 << 20);
	void* ptr = mmap(NULL, 
	                 numBytes, 
	                 PROT_READ | PROT_WRITE, 
	                 MAP_PRIVATE | MAP_ANONYMOUS | (usingHugePage ? MAP_HUGETLB : 0), 
	                 -1 /*fd*/, 
	                 0 /*offset*/);
	ReleaseAssert(ptr != MAP_FAILED);
	atype* a = reinterpret_cast<atype*>(ptr);
	printf("%d MB memory allocated at 0x%llx\n", memMB, ptr);
	rep(i, 0, numBytes - 1)
	{
		(reinterpret_cast<uint8_t*>(ptr))[i] = rand() % 256;
	}
	return a;
}

void DeallocateMemory(void* ptr, bool usingHugePage)
{
	uint64_t numBytes = memMB * uint64_t(1 << 20);
	int ret;
	if (usingHugePage)
	{
		ret = SAFE_HUGETLB_MUNMAP(ptr, numBytes);
	}
	else
	{
		ret = munmap(ptr, numBytes);
	}
	ReleaseAssert(ret == 0);
}

#define rotl64(x,r) ((x << r) | (x >> (64 - r)))
const uint64_t PRIME64_1 = 11400714785074694791ULL;

void RunOneBatchSize(int batchSize, atype* a, uint32_t* q, uint64_t& checksum, double& timeSpent)
{
	AutoTimer timer(&timeSpent);
	uint64_t sum = 0;
	int r = 0;
	int lim = numQueries / batchSize;
	uint32_t numEntries = memMB * uint64_t(1<<20) / 64;
	rep(i, 0, lim - 1)
	{
		uint64_t x = 0;
		uint64_t z = rotl64(sum, 23) * PRIME64_1;
		rep(j,0,batchSize - 1)
		{
			int pos = (q[r] ^ z) & (numEntries - 1);
			x += a[pos].value[0];
			r++;
		}
		sum += x;
	}
	checksum += sum;
}

const int numBatches = 29;
const int batchSizes[numBatches] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,36,48,72,96,120};

void RunTest(atype* a)
{
	uint32_t* q = new uint32_t[numQueries];
	ReleaseAssert(q != nullptr);
	Auto(delete [] q);
	rep(i,0,numQueries-1) q[i] = rand();
		
	uint64_t checksum = 0;
	double infLine;
	
	printf("Running test (infinite batch size)..\n");
	{
		AutoTimer timer(&infLine);
		uint32_t numEntries = memMB * uint64_t(1<<20) / 64;
		rep(i, 0, numQueries-1) checksum += a[q[i] & (numEntries-1)].value[0];
	}
	
	double res[numBatches];
	rep(id, 0, numBatches - 1)
	{
		int batchSize = batchSizes[id];
		printf("Running test (batch size = %d)\n", batchSize);
		{
			RunOneBatchSize(batchSize, a, q, checksum /*out*/, res[id] /*out*/); 
		}
	}
	
	printf("Test finished checksum = %llu\n", checksum);
	
	double normalized[numBatches];
	double baseline = res[0];
	rep(i, 0, numBatches - 1)
	{
		normalized[i] = res[i] * batchSizes[i] / baseline;
	}
	
	printf("baseline = %.3lf s (%.1lf ns / access)\n", baseline, 1e9 / (numQueries / baseline));
	printf("infLine = %.3lf s (MLP = %.3lf)\n", infLine, baseline / infLine);
	rep(i, 0, numBatches - 1)
	{
		printf("Batch size = %d, time = %.3lf s, equivalent cost = %.3lf, MLP = %.3lf\n", 
			batchSizes[i], res[i], normalized[i], batchSizes[i] / normalized[i]);
	}
}

TEST(DramSpeedTest, HugePage)
{
	atype* a = AllocateMemory(true /*usingHugePage*/);
	Auto(DeallocateMemory(a, true /*usingHugePage*/));
	
	RunTest(a);
}

TEST(DramSpeedTest, NoHugePage)
{
	atype* a = AllocateMemory(false /*usingHugePage*/);
	Auto(DeallocateMemory(a, false /*usingHugePage*/));
	
	RunTest(a);
}

}	// annoymous namespace

