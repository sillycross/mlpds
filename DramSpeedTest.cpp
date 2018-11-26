#include "common.h"
#include "gtest/gtest.h"

namespace {

struct atype
{
	uint64_t value[8];
};

static_assert(sizeof(atype) == 64, "size of atype should be 64");

const int memMB = 2048;		// must be power of 2
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
	for(uint64_t i = 0; i < numBytes; i++)
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

TEST(DramSpeedTest, HWAdjacentPrefetcher)
{
	printf("This test experiments with the effect of the adjacent line hardware prefetcher.\n");
	printf("This hardware prefetcher automatically prefetches adjacent cache \n");
	printf("line in the 128-byte block (the block is defined to start at a multiple of 128).\n");
	
	atype* a = AllocateMemory(true /*usingHugePage*/);
	Auto(DeallocateMemory(a, true /*usingHugePage*/));
	
	uint32_t* q = new uint32_t[numQueries];
	ReleaseAssert(q != nullptr);
	Auto(delete [] q);
	rep(i,0,numQueries-1) q[i] = rand();
		
	uint64_t checksum = 0;
	double infLine;
	
	int batchSize = 8;
	uint64_t sum = 0;
	printf("Running test (batch size = %d)\n", batchSize);
	
	printf("Plain version..\n");
	{
		AutoTimer timer;
		sum = 0;
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
	}
	printf("checksum = %llu\n", sum);
	
	printf("cache line shift = same cache line\n");
	printf("This measures the effect of an extra L1 hit\n");
	{
		AutoTimer timer;
		sum = 0;
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
			{
				int pos = (q[r-1-(sum & (batchSize-1))] ^ z) & (numEntries - 1);
				sum += a[pos].value[0];
			}
		}
	}
	printf("checksum = %llu\n", sum);
	
	printf("cache line shift = adjacent cache line in 128-byte block\n");
	printf("This measures the effect of the adjacent line HW prefetcher.\n");
	{
		AutoTimer timer;
		sum = 0;
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
			{
				int pos = ((q[r-1-(sum & (batchSize-1))] ^ z) ^ 1) & (numEntries - 1);
				sum += a[pos].value[0];
			}
		}
	}
	printf("checksum = %llu\n", sum);
	
	printf("cache line shift = adjacent cache line in different 128-byte block\n");
	printf("The adjacent HW prefetcher should not be helpful in this case.\n");
	{
		AutoTimer timer;
		sum = 0;
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
			{
				uint32_t idx = q[r-1-(sum & (batchSize-1))] ^ z;
				idx = (idx - 1) + (idx & 1) * 2;
				int pos = idx & (numEntries - 1);
				sum += a[pos].value[0];
			}
		}
	}
	printf("checksum = %llu\n", sum);
	
	printf("Now we shift by adding a speficied number of cache line.\n");
	printf("For cache line shift = 1, this should be equal to 50%% time HW prefetcher\n");
	printf("useful, and 50%% time HW prefetcher useless\n");
	printf("For cache line shift > 1, the HW prefetcher should always be useless\n");
	
	rep(cacheLineShift, 1, 7)
	{
		printf("cache line shift = %d\n", cacheLineShift);
		{
			AutoTimer timer;
			sum = 0;
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
				{
					int pos = ((q[r-1-(sum & (batchSize-1))] ^ z)+cacheLineShift) & (numEntries - 1);
					sum += a[pos].value[0];
				}
			}
		}
		printf("checksum = %llu\n", sum);
	}
}

TEST(DramSpeedTest, SumWithEarlyExitTest)
{
	atype* a = AllocateMemory(true /*usingHugePage*/);
	Auto(DeallocateMemory(a, true /*usingHugePage*/));
	
	// given 8 positions x_1, .. x_8
	// you try to find out the first i such that f(a[x_i]) is true
	// then sum up x_1 .. x_i 
	// what is the fastest way to do this?
	//
	
	uint32_t* q = new uint32_t[numQueries];
	ReleaseAssert(q != nullptr);
	Auto(delete [] q);
	rep(i,0,numQueries-1) q[i] = rand();
		
	uint64_t checksum = 0;
	double infLine;
	
	uint64_t sum;

	// break out chance at every step is chance_nume / chance_domi
	//
	const int chance_domi = 1024;
	
	printf("Always sum everything version..\n");
	{
		AutoTimer timer;
		sum = 0;
		uint32_t numEntries = memMB * uint64_t(1<<20) / 64;
		for (int i = 0; i < numQueries; i += 8)
		{
			uint64_t z = rotl64(sum, 23) * PRIME64_1;
			int pos[8];
			rep(k,0,7)
			{
				pos[k] = (q[i+k] ^ z) & (numEntries - 1);
			}
			rep(k, 0, 7)
			{
				sum += a[pos[k]].value[0];
			}
		}
	}
	printf("checksum = %llu\n", sum);
	
	printf("Always sum everything + prefetch version..\n");
	{
		AutoTimer timer;
		sum = 0;
		uint32_t numEntries = memMB * uint64_t(1<<20) / 64;
		for (int i = 0; i < numQueries; i += 8)
		{
			uint64_t z = rotl64(sum, 23) * PRIME64_1;
			int pos[8];
			rep(k,0,7)
			{
				pos[k] = (q[i+k] ^ z) & (numEntries - 1);
				MEM_PREFETCH(a[pos[k]].value[0]);
			}
			rep(k, 0, 7)
			{
				sum += a[pos[k]].value[0];
			}
		}
	}
	printf("checksum = %llu\n", sum);
	
	double results[9][3];
	
	rep(cid, 0, 8)
	{
		int chance_nume = cid * 128;
		chance_nume = min(chance_nume, 1023);
		chance_nume = max(chance_nume, 1);
		
		printf("Break out chance = %d / %d\n", chance_nume, chance_domi);
		
		printf("Simple branch version..\n");
		{
			AutoTimer timer(&results[cid][0]);
			sum = 0;
			uint32_t numEntries = memMB * uint64_t(1<<20) / 64;
			for (int i = 0; i < numQueries; i += 8)
			{
				uint64_t z = rotl64(sum, 23) * PRIME64_1;
				int pos[8];
				rep(k,0,7)
				{
					pos[k] = (q[i+k] ^ z) & (numEntries - 1);
				}
				rep(k, 0, 7)
				{
					sum += a[pos[k]].value[0];
					if (a[pos[k]].value[0] % chance_domi < chance_nume)
					{
						break;
					}
				}
			}
		}
		printf("checksum = %llu\n", sum);
		
		printf("Branchless avx version..\n");
		{
			AutoTimer timer(&results[cid][1]);
			sum = 0;
			uint32_t numEntries = memMB * uint64_t(1<<20) / 64;
			const __m128i idxMask = _mm_set1_epi32(numEntries - 1);
			const __m256i dataMask = _mm256_set1_epi64x(chance_domi - 1);
			const __m256i cmpv = _mm256_set1_epi64x(chance_nume - 1);
			for (int i = 0; i < numQueries; i += 8)
			{
				uint64_t z = rotl64(sum, 23) * PRIME64_1;
				uint32_t z32 = z;
				__m128i pos1 = _mm_loadu_si128(reinterpret_cast<__m128i const *>(&(q[i])));
				__m128i pos2 = _mm_loadu_si128(reinterpret_cast<__m128i const *>(&(q[i+4])));
				__m128i xorMask = _mm_set1_epi32(z32);
				pos1 = _mm_xor_si128(pos1, xorMask);
				pos1 = _mm_and_si128(pos1, idxMask);
				pos1 = _mm_slli_epi32(pos1, 3);
				pos2 = _mm_xor_si128(pos2, xorMask);
				pos2 = _mm_and_si128(pos2, idxMask);
				pos2 = _mm_slli_epi32(pos2, 3);
				__m256i data1 = _mm256_i32gather_epi64(reinterpret_cast<long long int const*>(a), pos1, 8);
				__m256i data2 = _mm256_i32gather_epi64(reinterpret_cast<long long int const*>(a), pos2, 8);
				__m256i cmp1 = _mm256_and_si256(data1, dataMask);
				__m256i cmp2 = _mm256_and_si256(data2, dataMask);
				cmp1 = _mm256_cmpgt_epi64(cmp1, cmpv);
				cmp2 = _mm256_cmpgt_epi64(cmp2, cmpv);
				int msk1 = _mm256_movemask_pd(_mm256_castsi256_pd(cmp1)) ^ 15;
				int msk2 = _mm256_movemask_pd(_mm256_castsi256_pd(cmp2)) ^ 15;
				int msk = msk1 | (msk2 << 4) | (1 << 7);
				int len = __builtin_ffs(msk);
				uint64_t tmp[8];
				_mm256_storeu_si256(reinterpret_cast<__m256i*>(tmp), data1);
				_mm256_storeu_si256(reinterpret_cast<__m256i*>(tmp + 4), data2);
				for (int k = 0; k < len; k++)
				{
					sum += tmp[k];
				}
			}
		}
		printf("checksum = %llu\n", sum);
		
		printf("Prefetch version..\n");
		{
			AutoTimer timer(&results[cid][2]);
			sum = 0;
			uint32_t numEntries = memMB * uint64_t(1<<20) / 64;
			for (int i = 0; i < numQueries; i += 8)
			{
				uint64_t x = 0;
				uint64_t z = rotl64(sum, 23) * PRIME64_1;
				int pos[8];
				rep(k,0,7)
				{
					pos[k] = (q[i+k] ^ z) & (numEntries - 1);
					MEM_PREFETCH(a[pos[k]].value[0]);
				}
				rep(k, 0, 7)
				{
					sum += a[pos[k]].value[0];
					if (a[pos[k]].value[0] % chance_domi < chance_nume)
					{
						break;
					}
				}
			}
		}
		printf("checksum = %llu\n", sum);
	}
	
	printf("Result stats:\n");
	printf("Simple branch:\n");
	rep(i, 0, 8)
	{
		printf("%.3lf ", results[i][0]);
	}
	printf("\n");
	
	printf("avx:\n");
	rep(i, 0, 8)
	{
		printf("%.3lf ", results[i][1]);
	}
	printf("\n");
	
	printf("prefetch:\n");
	rep(i, 0, 8)
	{
		printf("%.3lf ", results[i][2]);
	}
	printf("\n");
	
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

