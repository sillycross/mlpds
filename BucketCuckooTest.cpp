#include "common.h"
#include "gtest/gtest.h"

namespace BucketCuckoo
{
	
struct Bucket
{
	// 0 = empty, 1 = node, 2 = extending another node, 3 = slot reserved for insert
	//
	int exist[4];
	uint64_t value[4];
	
	Bucket() { memset(exist, 0, sizeof exist); }
	
	bool Exist(uint64_t v)
	{
		rep(i, 0, 3)
		{
			if (exist[i] == 1 && value[i] == v)
			{
				return true;
			}
		}
		return false;
	}
	
	int GetNumEmptyOrResevedForInsert()
	{
		int ret = 0;
		rep(i, 0, 3) if (exist[i] == 0 || exist[i] == 3) ret++;
		return ret;
	}
	
	int GetSize(int k)
	{
		int size = 1;
		k++;
		while (k < 4 && exist[k] == 2) k++, size++;
		return size;
	}
	
	pair<uint64_t, int> ClearItemForInsert(int which)
	{
		int size = GetSize(which);
		uint64_t v = value[which];
		rep(k, which, which + size - 1)
		{
			exist[k] = 3;
		}
		return make_pair(v, size);
	}
	
	pair<uint64_t, int> PopItemForInsert(uint64_t value, int size)
	{
		assert(GetNumEmptyOrResevedForInsert() < size);
		assert(1 <= size && size <= 3);
		// size == 1: always pop smallest size item
		// size == 2: if contains size = 3 item, pop it. Otherwise pop smallest size item
		// size == 3: pop largest size item
		//
		if (size == 1)
		{
			int minSize = 10000;
			int which = -1;
			rep(i, 0, 3)
			{
				if (exist[i] == 1 && GetSize(i) < minSize)
				{
					minSize = GetSize(i);
					which = i;
				}
			}
			assert(which != -1);
			return ClearItemForInsert(which);
		}
		if (size == 2)
		{
			int minSize = 10000;
			int which = -1;
			rep(i, 0, 3)
			{
				if (exist[i] == 1)
				{
					int x = GetSize(i);
					if (x == 3)
					{
						return ClearItemForInsert(i);
					}
					if (x < minSize)
					{
						minSize = GetSize(i);
						which = i;
					}
				}
			}
			assert(which != -1);
			return ClearItemForInsert(which);
		}
		if (size == 3)
		{
			int maxSize = 0;
			int which = -1;
			rep(i, 0, 3)
			{
				if (exist[i] == 1 && GetSize(i) > maxSize)
				{
					maxSize = GetSize(i);
					which = i;
				}
			}
			assert(which != -1);
			return ClearItemForInsert(which);
		}
	}
	
	void InsertItem(uint64_t v, int size)
	{
		assert(GetNumEmptyOrResevedForInsert() >= size);
		int c = 0;
		int k = 0;
		while (k < 4)
		{
			if (exist[k] == 1)
			{
				int t = GetSize(k);
				exist[c] = exist[k];
				value[c] = value[k];
				c++;
				rep(i, 1, t-1) 
				{
					exist[c] = 2;
					c++;
				}
				k += t;
			}
			else
			{
				k++;
			}
		}
		assert(c+size-1 <= 3);
		rep(i, c, 3)
		{
			exist[i] = 0;
		}
		exist[c] = 1;
		value[c] = v;
		rep(i, c+1, c+size-1)
		{
			exist[i] = 2;
		}
	}		
};

namespace XXH
{

static const uint32_t XXH_SEED1 = 1192827283U;
static const uint32_t XXH_SEED2 = 534897851U;

static const uint32_t PRIME32_1 = 2654435761U;
static const uint32_t PRIME32_2 = 2246822519U;
static const uint32_t PRIME32_3 = 3266489917U;
static const uint32_t PRIME32_4 = 668265263U;
static const uint32_t PRIME32_5 = 374761393U;

#define XXH_rotl32(x,r) ((x << r) | (x >> (32 - r)))

uint32_t XXH32_avalanche(uint32_t h32)
{
    h32 ^= h32 >> 15;
    h32 *= PRIME32_2;
    h32 ^= h32 >> 13;
    h32 *= PRIME32_3;
    h32 ^= h32 >> 16;
    return h32;
}

uint32_t XXH32_CoreLogic(uint64_t key, uint32_t len, uint32_t seed, uint32_t multiplier)
{
	key >>= (8-len)*8;
	key <<= (8-len)*8;
	uint32_t low = key;
	uint32_t high = key >> 32;
	uint32_t h32 = PRIME32_5 + seed + len;
	h32 ^= high * multiplier; 
	h32 = XXH_rotl32(h32, 17) * PRIME32_4;
	if (len > 4)
	{
		h32 ^= low * multiplier; 
		h32 = XXH_rotl32(h32, 17) * PRIME32_4;
	}
	return XXH32_avalanche(h32);
}

uint32_t HashFn1(uint64_t key, uint32_t len)
{
	return XXH32_CoreLogic(key, len, XXH_SEED1, PRIME32_1);
}

uint32_t HashFn2(uint64_t key, uint32_t len)
{
	return XXH32_CoreLogic(key, len, XXH_SEED2, PRIME32_3);
}

uint32_t HashFn3(uint64_t key, uint32_t len)
{
	return XXH32_CoreLogic(key, len, 0 /*seed*/, PRIME32_3);
}

}	// namespace XXH

const int HtSize = 8388608;

Bucket ht[HtSize];

int CuckooDisplacement(uint32_t pos, uint64_t value, int size, int round, bool& failed)
{
	if (round > 1000)
	{
		failed = true;
		return 0;
	}
	int sumMoves = 0;
	while (ht[pos].GetNumEmptyOrResevedForInsert() < size)
	{
		sumMoves++;
		pair<uint64_t, int> victim = ht[pos].PopItemForInsert(value, size);
		uint32_t h1 = XXH::HashFn1(victim.first, 8) % HtSize;
		uint32_t h2 = XXH::HashFn2(victim.first, 8) % HtSize;
		if (h1 == h2) { h2 = (h2 + 1) % HtSize; }
		assert(h1 == pos || h2 == pos);
		if (h1 == pos) h1 = h2;
		assert(h1 != pos);
		sumMoves += CuckooDisplacement(h1, victim.first, victim.second, round + 1, failed);
		if (failed)
		{
			return 0;
		}
	}
	ht[pos].InsertItem(value, size);
	return sumMoves;
}

int histogram[1010];

void Insert(uint64_t value, int size, bool& exist, bool& failed)
{
	failed = false;
	exist = false;
	uint32_t h1 = XXH::HashFn1(value, 8) % HtSize;
	uint32_t h2 = XXH::HashFn2(value, 8) % HtSize;
	if (h1 == h2) { h2 = (h2 + 1) % HtSize; }
	if (ht[h1].Exist(value) || ht[h2].Exist(value))
	{
		exist = true;
		return;
	}
	int h1x = ht[h1].GetNumEmptyOrResevedForInsert();
	int h2x = ht[h2].GetNumEmptyOrResevedForInsert();
	if (h1x >= size)
	{
		ht[h1].InsertItem(value, size);
		histogram[0]++;
		return;
	}
	if (h2x >= size)
	{
		ht[h2].InsertItem(value, size);
		histogram[0]++;
		return;
	}
	if (h1x < h2x)
	{
		h1 = h2;
	}
	int numMoves = CuckooDisplacement(h1, value, size, 0, failed);
	if (failed)
	{
		return;
	}
	histogram[numMoves]++;
	return;
}

int numInsertions;
int numSlotsUsed;
int numEquivalentSetSize;

double prob[3] = {0.88, 0.1, 0.02};
int equivSetSize[3] = {1, 5, 20};

void GenerateAndInsertData()
{
	double x = double(rand()) / RAND_MAX;
	int sz;
	rep(k, 0, 2)
	{
		if (x < prob[k] || k == 2)
		{
			sz = k+1;
			break;
		}
		else
		{
			x -= prob[k];
		}
	}
	uint64_t value = 0;
	rep(k, 0, 7)
	{
		value = value * 256 + rand() % 256;
	}
	bool exist, failed;
	Insert(value, sz, exist, failed);
	if (failed)
	{
		printf("failed at %d slots used, lf = %.3lf%%, %d equivalent set size\n", 
			numSlotsUsed, double(numSlotsUsed) / HtSize / 4, numEquivalentSetSize / 2);
		exit(0);
	}
	if (exist) return;
	numInsertions++;
	numSlotsUsed += sz;
	numEquivalentSetSize += equivSetSize[sz-1];
}

TEST(BucketCuckooTest, InsertLimitTest)
{
	numInsertions = 0;
	numSlotsUsed = 0;
	numEquivalentSetSize = 0;
	while (1)
	{
		GenerateAndInsertData();
	}
}

TEST(BucketCuckooTest, InsertDisplacementHistogram)
{
	numSlotsUsed = 0;
	numEquivalentSetSize = 0;
	while (numSlotsUsed < 0.9 * HtSize * 4)
	{
		GenerateAndInsertData();
	}
	printf("num insertions = %d, num slots used = %d\n", numInsertions, numSlotsUsed);
	int k = 1000;
	while (histogram[k] == 0) k--;
	LL sum = 0;
	rep(i, 0, k)
	{
		printf("%d: %d\n", i, histogram[i]);
		sum += LL(i) * histogram[i];
	}
	printf("avg = %.3lf\n", double(sum) / numInsertions); 
}

}	// namespace BucketCuckoo

