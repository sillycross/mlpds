#include "MlpSetUInt64.h"

namespace MlpSetUInt64
{

// Vectorized XXHash utility
// The hash function is a slightly modified XXH32, to fix a known deficiency 
// The 3 hash functions are XXHashFn1, XXHashFn2, XXHashFn3
// XXHashArray computes 18 hashes from the prefixes of a key 
// using vectorization (detail in function comment)
//
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

uint32_t XXHashFn1(uint64_t key, uint32_t len)
{
	return XXH32_CoreLogic(key, len, XXH_SEED1, PRIME32_1);
}

uint32_t XXHashFn2(uint64_t key, uint32_t len)
{
	return XXH32_CoreLogic(key, len, XXH_SEED2, PRIME32_3);
}

uint32_t XXHashFn3(uint64_t key, uint32_t len)
{
	return XXH32_CoreLogic(key, len, 0 /*seed*/, PRIME32_3);
}

static const __m128i PRIME32_1_ARRAY = _mm_set_epi32(PRIME32_1, PRIME32_1, PRIME32_1, PRIME32_1);
static const __m128i PRIME32_2_ARRAY = _mm_set_epi32(PRIME32_2, PRIME32_2, PRIME32_2, PRIME32_2);
static const __m128i PRIME32_3_ARRAY = _mm_set_epi32(PRIME32_3, PRIME32_3, PRIME32_3, PRIME32_3);
static const __m128i PRIME32_4_ARRAY = _mm_set_epi32(PRIME32_4, PRIME32_4, PRIME32_4, PRIME32_4);

static const __m128i XXH_OUT1_INIT = _mm_set_epi32(PRIME32_5 + XXH_SEED1 + 8U,
                                                   PRIME32_5 + XXH_SEED1 + 7U,
                                                   PRIME32_5 + XXH_SEED1 + 6U,
                                                   PRIME32_5 + XXH_SEED1 + 5U);
static const __m128i XXH_OUT2_INIT = _mm_set_epi32(PRIME32_5 + XXH_SEED2 + 8U,
                                                   PRIME32_5 + XXH_SEED2 + 7U,
                                                   PRIME32_5 + XXH_SEED2 + 6U,
                                                   PRIME32_5 + XXH_SEED2 + 5U);
static const __m128i XXH_OUT3_INIT = _mm_set_epi32(PRIME32_5 + 8U,
                                                   PRIME32_5 + 7U,
                                                   PRIME32_5 + 6U,
                                                   PRIME32_5 + 5U);
static const __m128i XXH_OUT4_INIT = _mm_set_epi32(PRIME32_5 + XXH_SEED1 + 4U,
                                                   PRIME32_5 + XXH_SEED1 + 3U,
                                                   PRIME32_5 + XXH_SEED2 + 4U,
                                                   PRIME32_5 + XXH_SEED2 + 3U);
static const __m128i XXH_LOW_MASK = _mm_set_epi32(0xffffffffU, 
                                                  0xffffff00U, 
                                                  0xffff0000U, 
                                                  0xff000000U);

void XXHExecuteRotlAndMult(__m128i& data)
{
	__m128i tmp = _mm_srli_epi32(data, 15);
	data = _mm_slli_epi32(data, 17);
	data = _mm_or_si128(data, tmp);
	data = _mm_mullo_epi32(data, PRIME32_4_ARRAY);
}

void XXHExecuteAvalanche(__m128i& data)
{
	__m128i tmp = _mm_srli_epi32(data, 15);
	data = _mm_xor_si128(data, tmp);
	data = _mm_mullo_epi32(data, PRIME32_2_ARRAY);
	
	tmp = _mm_srli_epi32(data, 13);
	data = _mm_xor_si128(data, tmp);
	data = _mm_mullo_epi32(data, PRIME32_3_ARRAY);
	
	tmp = _mm_srli_epi32(data, 16);
	data = _mm_xor_si128(data, tmp);
}

// high to low:
// out1: h1(8), h1(7), h1(6), h1(5)
// out2: h2(8), h2(7), h2(6), h2(5)
// out3: h3(8), h3(7), h3(6), h3(5)
// out4: h1(4), h1(3), h2(4), h2(3)
// out5: h3(4), h3(3)
//
void XXHashArray(uint64_t key, __m128i& out1, __m128i& out2, __m128i& out3, __m128i& out4, uint64_t& out5)
{
	uint32_t low = key;
	uint32_t high = key >> 32;
	
	uint32_t x1 = high * PRIME32_1;
	uint32_t x2 = high * PRIME32_3;
	
	out1 = _mm_set1_epi32(x1);
	out1 = _mm_xor_si128(out1, XXH_OUT1_INIT);
	XXHExecuteRotlAndMult(out1);
	
	out2 = _mm_set1_epi32(x2);
	out2 = _mm_xor_si128(out2, XXH_OUT2_INIT);
	XXHExecuteRotlAndMult(out2);
	
	out3 = _mm_set1_epi32(x2);
	out3 = _mm_xor_si128(out3, XXH_OUT3_INIT);
	XXHExecuteRotlAndMult(out3);
	
	uint32_t high3byte = high & 0xffffff00U;
	uint32_t high3byteP1 = high3byte * PRIME32_1;
	uint32_t high3byteP3 = high3byte * PRIME32_3;
	
	out4 = _mm_set_epi32(x1, high3byteP1, x2, high3byteP3);
	out4 = _mm_xor_si128(out4, XXH_OUT4_INIT);
	XXHExecuteRotlAndMult(out4);
	
	__m128i v1 = _mm_set1_epi32(low);
	v1 = _mm_and_si128(v1, XXH_LOW_MASK);
	__m128i v2 = _mm_mullo_epi32(v1, PRIME32_1_ARRAY);
	__m128i v3 = _mm_mullo_epi32(v1, PRIME32_3_ARRAY);
	
	out1 = _mm_xor_si128(out1, v2);
	XXHExecuteRotlAndMult(out1);
	
	out2 = _mm_xor_si128(out2, v3);
	XXHExecuteRotlAndMult(out2);
	
	out3 = _mm_xor_si128(out3, v3);
	XXHExecuteRotlAndMult(out3);
	
	XXHExecuteAvalanche(out1);
	XXHExecuteAvalanche(out2);
	XXHExecuteAvalanche(out3);
	XXHExecuteAvalanche(out4);
	
	uint32_t h34 = (PRIME32_5 + 4U) ^ x2;
	h34 = XXH_rotl32(h34, 17) * PRIME32_4;
	h34 = XXH32_avalanche(h34);
	
	uint32_t h33 = (PRIME32_5 + 3U) ^ high3byteP3;
	h33 = XXH_rotl32(h33, 17) * PRIME32_4;
	h33 = XXH32_avalanche(h33);
	
	assert(_mm_extract_epi32(out1, 3) == XXHashFn1(key, 8));
	assert(_mm_extract_epi32(out1, 2) == XXHashFn1(key, 7));
	assert(_mm_extract_epi32(out1, 1) == XXHashFn1(key, 6));
	assert(_mm_extract_epi32(out1, 0) == XXHashFn1(key, 5));
	assert(_mm_extract_epi32(out2, 3) == XXHashFn2(key, 8));
	assert(_mm_extract_epi32(out2, 2) == XXHashFn2(key, 7));
	assert(_mm_extract_epi32(out2, 1) == XXHashFn2(key, 6));
	assert(_mm_extract_epi32(out2, 0) == XXHashFn2(key, 5));
	assert(_mm_extract_epi32(out3, 3) == XXHashFn3(key, 8));
	assert(_mm_extract_epi32(out3, 2) == XXHashFn3(key, 7));
	assert(_mm_extract_epi32(out3, 1) == XXHashFn3(key, 6));
	assert(_mm_extract_epi32(out3, 0) == XXHashFn3(key, 5));
	assert(_mm_extract_epi32(out4, 3) == XXHashFn1(key, 4));
	assert(_mm_extract_epi32(out4, 2) == XXHashFn1(key, 3));
	assert(_mm_extract_epi32(out4, 1) == XXHashFn2(key, 4));
	assert(_mm_extract_epi32(out4, 0) == XXHashFn2(key, 3));
	assert(h34 == XXHashFn3(key, 4));
	assert(h33 == XXHashFn3(key, 3));
	
	out5 = (uint64_t(h34) << 32) | h33;
}

}	// namespace XXH
	
void CuckooHashTableNode::Init(int ilen, int dlen, uint64_t dkey, uint32_t hash18bit, int firstChild)
{
	assert(!IsOccupied());
	assert(1 <= ilen && ilen <= 8 && 1 <= dlen && dlen <= 8 && -1 <= firstChild && firstChild <= 255);
	hash = 0x80000000U | ((ilen - 1) << 27) | ((dlen - 1) << 24) | hash18bit;
	minKey = dkey;
	childMap = firstChild;
}
	
int CuckooHashTableNode::FindNeighboringEmptySlot()
{
	int lowbits = reinterpret_cast<uintptr_t>(this) & 63;
	if (lowbits < 32)
	{	
		// favors same cache line slot most
		//
		if (!this[1].IsOccupied())
		{
			return 1;
		}
		else if (!this[-1].IsOccupied())
		{
			return -1;
		}
	}
	rep(i, 1, 3)
	{
		if (!this[i].IsOccupied())
		{
			return i;
		}
		if (!this[-i].IsOccupied())
		{
			return -i;
		}
	}
	return 0;
}
	
void CuckooHashTableNode::BitMapSet(int child)
{
	assert(IsNode() && !IsLeaf() && !IsUsingInternalChildMap());
	assert(0 <= child && child <= 255);
	if (unlikely(IsExternalPointerBitMap()))
	{
		uint64_t* ptr = reinterpret_cast<uint64_t*>(childMap);
		ptr[child / 64] |= uint64_t(1) << (child % 64);
	}
	else
	{
		if (child < 64)
		{
			childMap |= uint64_t(1) << child;
		}
		else
		{
			if (child == 94 || child == 95)
			{
				hash |= 1 << (child - 76);
			}
			else
			{
				int offset = ((hash >> 21) & 7) - 4;
				uint64_t* ptr = reinterpret_cast<uint64_t*>(&(this[offset]));
				ptr[child / 64 - 1] |= uint64_t(1) << (child % 64);
			}
		}
	}
}
	
uint64_t* CuckooHashTableNode::AllocateExternalBitMap()
{
	uint64_t* ptr = new uint64_t[4];
	memset(ptr, 0, 32);
	return ptr;
}
	
void CuckooHashTableNode::ExtendToBitMap()
{
	assert(IsNode() && !IsLeaf() && IsUsingInternalChildMap() && GetChildNum() == 8);
	uint64_t children = childMap;
	int offset = FindNeighboringEmptySlot() + 4;
	assert(1 <= offset && offset <= 7);
	hash &= 0xff03ffffU;
	hash |= (offset << 21);
	if (offset == 4)
	{
		uint64_t* ptr = AllocateExternalBitMap();
		childMap = reinterpret_cast<uintptr_t>(ptr);
	}
	else
	{
		childMap = 0;
		uint64_t* ptr = reinterpret_cast<uint64_t*>(&(this[offset-4]));
		memset(ptr, 0, sizeof(CuckooHashTableNode));
		this[offset-4].hash = 0xc0000000U;
	}
	
	rep(i,0,7)
	{
		int child = children & 255;
		children >>= 8;
		BitMapSet(child);
	}
}

static int Bitmap256LowerBound(uint64_t* ptr, uint32_t child)
{
	assert(0 <= child && child <= 255);
	int idx = child / 64;
	uint64_t x = ptr[idx] >> (child % 64);
	if (x) 
	{ 
		return __builtin_ctzll(x) + child; 
	}
	idx++;
	while (idx < 4)
	{
		if (ptr[idx] != 0)
		{
			return __builtin_ctzll(ptr[idx]) + idx * 64;
		}
		idx++;
	}
	return -1;
}

int CuckooHashTableNode::LowerBoundChild(uint32_t child)
{
	assert(IsNode() && !IsLeaf());
	assert(0 <= child && child <= 255);
	if (IsUsingInternalChildMap())
	{
		if (child == 0) { return childMap & 255; }
		int k = GetChildNum();
		__m64 z = _mm_cvtsi64_m64(childMap);
		__m64 cmpTarget = _mm_set1_pi8(child - 1);
		__m64 res = _mm_max_pu8(cmpTarget, z);
		res = _mm_cmpeq_pi8(cmpTarget, res);
		int msk = _mm_movemask_pi8(res);
		msk &= (1<<k)-1;
		msk++;
		int pos = __builtin_ffs(msk);
		assert(1 <= pos && pos <= k + 1);
		if (pos == k+1) 
		{ 
			return -1; 
		}
		return (childMap >> ((pos-1)*8)) & 255;
	}
	else if (unlikely(IsExternalPointerBitMap()))
	{
		uint64_t* ptr = reinterpret_cast<uint64_t*>(childMap);
		return Bitmap256LowerBound(ptr, child);
	}
	else
	{
		int offset = (hash >> 21) & 7;
		if (child < 64)
		{
			uint64_t x = childMap >> child;
			if (x)
			{	
				return __builtin_ctzll(x) + child; 
			}
			uint64_t* ptr = reinterpret_cast<uint64_t*>(&(this[offset-4]));
			x = ptr[0] & 0xffffffff3fffffffULL;
			x |= uint64_t((hash >> 18) & 3) << 30;
			if (x)
			{
				return __builtin_ctzll(x) + 64;
			}
			rep(k, 1, 2)
			{
				if (ptr[k])
				{
					return __builtin_ctzll(ptr[k]) + (k+1) * 64;
				}
			}
			return -1;
		}
		else if (child < 128)
		{
			uint64_t* ptr = reinterpret_cast<uint64_t*>(&(this[offset-4]));
			uint64_t x = ptr[0] & 0xffffffff3fffffffULL;
			x |= uint64_t((hash >> 18) & 3) << 30;
			x >>= (child - 64);
			if (x)
			{
				return __builtin_ctzll(x) + child;
			}
			rep(k, 1, 2)
			{
				if (ptr[k])
				{
					return __builtin_ctzll(ptr[k]) + (k+1) * 64;
				}
			}
			return -1;
		}
		else
		{
			uint64_t* ptr = reinterpret_cast<uint64_t*>(&(this[offset-4]));
			int idx = child / 64 - 1;
			uint64_t x = ptr[idx] >> (child % 64);
			if (x)
			{
				return __builtin_ctzll(x) + child;
			}
			if (idx < 2)
			{
				if (ptr[2])
				{
					return __builtin_ctzll(ptr[2]) + 192;
				}
			}
			return -1;
		}
	}	
}
	
bool CuckooHashTableNode::ExistChild(int child)
{
	assert(IsNode() && !IsLeaf());
	assert(0 <= child && child <= 255);
	if (IsUsingInternalChildMap())
	{
		int k = GetChildNum();
		__m64 z = _mm_cvtsi64_m64(childMap);
		__m64 cmpTarget = _mm_set1_pi8(child);
		__m64 res = _mm_cmpeq_pi8(cmpTarget, z);
		int msk = _mm_movemask_pi8(res);
		msk &= (1<<k)-1;
		bool result = (msk != 0);
		
#ifndef NDEBUG
		bool bruteForceResult = false;
		uint64_t c = childMap;
		rep(i,0,k-1)
		{
			int x = c & 255;
			c >>= 8;
			if (x == child) { bruteForceResult = true; break; }
		}
		assert(result == bruteForceResult);
#endif
		return result;
	}
	else if (unlikely(IsExternalPointerBitMap()))
	{
		uint64_t* ptr = reinterpret_cast<uint64_t*>(childMap);
		return (ptr[child / 64] & (uint64_t(1) << (child % 64))) != 0;
	}
	else
	{
		if (child < 64)
		{
			return (childMap & (uint64_t(1) << child)) != 0;
		}
		else if (unlikely(child == 94 || child == 95))
		{
			return (hash & (1 << (child - 76))) != 0;
		}
		else
		{
			int offset = ((hash >> 21) & 7) - 4;
			uint64_t* ptr = reinterpret_cast<uint64_t*>(&(this[offset]));
			return (ptr[child / 64 - 1] & (uint64_t(1) << (child % 64))) != 0;
		}
	}
}

void CuckooHashTableNode::AddChild(int child)
{
	assert(IsNode() && !IsLeaf());
	assert(0 <= child && child <= 255);
	assert(!ExistChild(child));
	if (IsUsingInternalChildMap())
	{
		int k = GetChildNum();
		if (likely(k < 8))
		{
			SetChildNum(k+1);
			__m64 z = _mm_cvtsi64_m64(childMap);
			__m64 cmpTarget = _mm_set1_pi8(child);
			__m64 res = _mm_max_pu8(cmpTarget, z);
			res = _mm_cmpeq_pi8(cmpTarget, res);
			int msk = _mm_movemask_pi8(res);
			msk &= (1<<k)-1;
			msk++;
			int pos = __builtin_ffs(msk);
			assert(1 <= pos && pos <= k + 1);
			uint64_t larger = (pos == 8) ? 0 : (childMap >> ((pos-1)*8) << (pos*8));
			uint64_t smaller = childMap & ((uint64_t(1) << ((pos-1)*8)) - 1);
			childMap = smaller | (uint64_t(child) << ((pos - 1)*8)) | larger;
#ifndef NDEBUG
			uint64_t tmp = childMap;
			int last = tmp % 256;
			rep(i, 1, k)
			{
				tmp /= 256;
				int cur = tmp % 256;
				assert(cur > last);
				last = cur;
			}
#endif
			return;
		}
		ExtendToBitMap();
	}
	BitMapSet(child);
}

vector<int> CuckooHashTableNode::GetAllChildren()
{
	assert(IsNode());
	if (IsLeaf())
	{
		return vector<int>();
	}
	
	vector<int> ret;
	if (IsUsingInternalChildMap())
	{
		uint64_t c = childMap;
		int k = GetChildNum();
		rep(i,0,k-1)
		{
			ret.push_back(c & 255);
			c >>= 8;
		}
		rep(i, 1, k-1)
		{
			assert(ret[i] > ret[i-1]);
		}
	}
	else if (unlikely(IsExternalPointerBitMap()))
	{
		uint64_t* ptr = reinterpret_cast<uint64_t*>(childMap);
		rep(i,0,255)
		{
			if (ptr[i/64] & (uint64_t(1) << (i%64)))
			{
				ret.push_back(i);
			}
		}
	}
	else
	{
		rep(i,0,63)
		{
			if (childMap & (uint64_t(1) << i))
			{
				ret.push_back(i);
			}
		}
		int offset = ((hash >> 21) & 7) - 4;
		uint64_t* ptr = reinterpret_cast<uint64_t*>(&(this[offset]));
		rep(i, 64, 93)
		{
			if (ptr[i/64-1] & (uint64_t(1) << (i%64)))
			{
				ret.push_back(i);
			}
		}
		rep(i, 94, 95)
		{
			if (hash & (1 << (i - 76)))
			{
				ret.push_back(i);
			}
		}
		rep(i, 96, 255)
		{
			if (ptr[i/64-1] & (uint64_t(1) << (i%64)))
			{
				ret.push_back(i);
			}
		}
	}
	return ret;
}
	
uint64_t* CuckooHashTableNode::CopyToExternalBitMap()
{
	assert(IsNode() && !IsLeaf() && !IsUsingInternalChildMap() && !IsExternalPointerBitMap());
	uint64_t* ptr = AllocateExternalBitMap();
	int offset = (hash >> 21) & 7;
	ptr[0] = childMap;
	memcpy(ptr+1, &(this[offset-4]), sizeof(CuckooHashTableNode));
	ptr[1] &= 0xffffffff3fffffffULL;
	ptr[1] |= uint64_t((hash >> 18) & 3) << 30;
	return ptr;
}
	
void CuckooHashTableNode::MoveNode(CuckooHashTableNode* target)
{
	*target = *this;
	if (IsUsingInternalChildMap() || IsExternalPointerBitMap())
	{
		memset(this, 0, sizeof(CuckooHashTableNode));
		return;
	}
	int offset = (hash >> 21) & 7;
	int targetOffset = target->FindNeighboringEmptySlot() + 4;
	target->hash &= 0xff1fffffU;
	target->hash |= (targetOffset << 21);
	if (targetOffset != 4)
	{
		memcpy(&(target[targetOffset-4]), &(this[offset-4]), sizeof(CuckooHashTableNode));
	}
	else
	{
		uint64_t* ptr = CopyToExternalBitMap();
		target->childMap = reinterpret_cast<uint64_t>(ptr);
	}
	memset(this, 0, sizeof(CuckooHashTableNode));
	memset(&(this[offset-4]), 0, sizeof(CuckooHashTableNode));
#ifndef NDEBUG
	assert(target->IsNode());
	if (!target->IsUsingInternalChildMap() && !target->IsExternalPointerBitMap())
	{
		int o = (target->hash >> 21) & 7;
		assert(target[o-4].IsOccupied() && !target[o-4].IsNode());
	}
#endif
}
	
void CuckooHashTableNode::RelocateBitMap()
{
	assert(IsNode() && !IsLeaf() && !IsUsingInternalChildMap() && !IsExternalPointerBitMap());
	uint64_t children = childMap;
	int offset = FindNeighboringEmptySlot() + 4;
	assert(1 <= offset && offset <= 7);
	int oldOffset = (hash >> 21) & 7;
	assert(offset != oldOffset);
	if (offset == 4)
	{
		uint64_t* ptr = CopyToExternalBitMap();
		childMap = reinterpret_cast<uint64_t>(ptr);
	}
	else
	{
		memcpy(&(this[offset-4]), &(this[oldOffset-4]), sizeof(CuckooHashTableNode));
	}
	memset(&(this[oldOffset-4]), 0, sizeof(CuckooHashTableNode));
	hash &= 0xfff1fffffU;
	hash |= offset << 21;
	assert(offset == 4 || (this[offset-4].IsOccupied() && !this[offset-4].IsNode()));
}	

static const __m128i HASH18_MASK = _mm_set_epi32(0x3ffffU, 0x3ffffU, 0x3ffffU, 0x3ffffU);
static const __m128i HASH_EXPECT_MASK1 = _mm_set_epi32(0x80000000U | (7U << 27), 
                                                       0x80000000U | (6U << 27), 
                                                       0x80000000U | (5U << 27), 
                                                       0x80000000U | (4U << 27));
static const __m128i HASH_EXPECT_MASK2 = _mm_set_epi32(0xf803ffffU, 0xf803ffffU, 0xf803ffffU, 0xf803ffffU);

static inline uint64_t RoundUpToNearestMultipleOf(uint64_t x, uint64_t y)
{
	if (x % y == 0) return x;
	return x / y * y + y;
}

static inline uint64_t RoundUpToNearestPowerOf2(uint64_t x)
{
	uint64_t z = 1;
	while (z < x) z *= 2;
	return z;
}

static inline void MultiplyBy3(const __m128i& input, __m128i& output)
{
	output = _mm_add_epi32(input, input);
	output = _mm_add_epi32(input, output);
}

#ifdef ENABLE_STATS
CuckooHashTable::Stats::Stats()
	: m_slowpathCount(0)
	, m_movedNodesCount(0)
	, m_relocatedBitmapsCount(0)
{
	memset(m_lcpResultHistogram, 0, sizeof m_lcpResultHistogram); 
}

void CuckooHashTable::Stats::ClearStats()
{
	m_slowpathCount = 0;
	m_movedNodesCount = 0;
	m_relocatedBitmapsCount = 0;
	memset(m_lcpResultHistogram, 0, sizeof m_lcpResultHistogram); 
}

void CuckooHashTable::Stats::ReportStats()
{
	printf("Cuckoo HashTable stats:\n");
	printf("\tQueryLCP slow-path count = %u\n", m_slowpathCount);
	printf("\tInsertion moved nodes count = %u\n", m_movedNodesCount);
	printf("\tInsertion relocated bitmaps count = %u\n", m_relocatedBitmapsCount);
	printf("\tQueryLCP result histogram (result node IndexLen, not actual LCP):\n");
	rep(i, 2, 8)
	{
		printf("\t\tLCP = %d: %u\n", i, m_lcpResultHistogram[i]);
	}
}
#endif

CuckooHashTable::CuckooHashTable() 
	: ht(nullptr)
	, htMask(0)
#ifdef ENABLE_STATS
	, stats()
#endif
#ifndef NDEBUG
	, m_hasCalledInit(false)
#endif
{ }
	
void CuckooHashTable::Init(CuckooHashTableNode* _ht, uint64_t _mask)
{
	assert(!m_hasCalledInit);
#ifndef NDEBUG
	m_hasCalledInit = true;
#endif
	ht = _ht;
	htMask = _mask;
	assert(reinterpret_cast<uintptr_t>(_ht) % 128 == 0);
	assert(RoundUpToNearestPowerOf2(_mask + 1) == _mask + 1);
}

uint32_t CuckooHashTable::ReservePositionForInsert(int ilen, uint64_t dkey, uint32_t hash18bit, bool& exist, bool& failed)
{
	assert(m_hasCalledInit);
	
	exist = false;
	failed = false;
	
	uint32_t expectedHash = hash18bit | ((ilen-1) << 27) | 0x80000000U;
	int shiftLen = 64 - 8 * ilen;
	uint64_t shiftedKey = dkey >> shiftLen;
	
	uint32_t h1, h2;
	h1 = XXH::XXHashFn1(dkey, ilen) & htMask;
	h2 = XXH::XXHashFn2(dkey, ilen) & htMask;
	if (ht[h1].IsEqual(expectedHash, shiftLen, shiftedKey))
	{
		exist = true;
		return h1;
	}
	if (ht[h2].IsEqual(expectedHash, shiftLen, shiftedKey))
	{
		exist = true;
		return h2;
	}
	if (!ht[h1].IsOccupied())
	{
		return h1;
	}
	if (!ht[h2].IsOccupied())
	{
		return h2;
	}
	uint32_t victimPosition = rand()%2 ? h1 : h2;
	HashTableCuckooDisplacement(victimPosition, 1, failed);
	if (failed)
	{
		return -1;
	}
	assert(!ht[victimPosition].IsOccupied());
	return victimPosition;
}

uint32_t CuckooHashTable::Insert(int ilen, int dlen, uint64_t dkey, int firstChild, bool& exist, bool& failed)
{
	assert(m_hasCalledInit);
	
	uint32_t hash18bit = XXH::XXHashFn3(dkey, ilen);
	hash18bit = hash18bit & ((1<<18) - 1);
	
	uint32_t pos = ReservePositionForInsert(ilen, dkey, hash18bit, exist, failed);
	if (!exist && !failed)
	{
		ht[pos].Init(ilen, dlen, dkey, hash18bit, firstChild);
	}
	return pos;
}

uint32_t CuckooHashTable::Lookup(int ilen, uint64_t ikey, bool& found)
{
	assert(m_hasCalledInit);
	
	found = false;
	uint32_t hash18bit = XXH::XXHashFn3(ikey, ilen);
	hash18bit = hash18bit & ((1<<18) - 1);
	uint32_t expectedHash = hash18bit | ((ilen-1) << 27) | 0x80000000U;
	int shiftLen = 64 - 8 * ilen;
	uint64_t shiftedKey = ikey >> shiftLen;
	
	uint32_t h1, h2;
	h1 = XXH::XXHashFn1(ikey, ilen) & htMask;
	h2 = XXH::XXHashFn2(ikey, ilen) & htMask;
	MEM_PREFETCH(ht[h1]);
	MEM_PREFETCH(ht[h2]);
	if (ht[h1].IsEqual(expectedHash, shiftLen, shiftedKey))
	{
		found = true;
		return h1;
	}
	if (ht[h2].IsEqual(expectedHash, shiftLen, shiftedKey))
	{
		found = true;
		return h2;
	}
	return -1;
}

CuckooHashTable::LookupMustExistPromise CuckooHashTable::GetLookupMustExistPromise(int ilen, uint64_t ikey)
{
	assert(m_hasCalledInit);
	
	uint32_t hash18bit = XXH::XXHashFn3(ikey, ilen);
	hash18bit = hash18bit & ((1<<18) - 1);
	uint32_t expectedHash = hash18bit | ((ilen-1) << 27) | 0x80000000U;
	int shiftLen = 64 - 8 * ilen;
	uint64_t shiftedKey = ikey >> shiftLen;
	
	uint32_t h1, h2;
	h1 = XXH::XXHashFn1(ikey, ilen) & htMask;
	h2 = XXH::XXHashFn2(ikey, ilen) & htMask;
	
	return LookupMustExistPromise(true /*valid*/,
	                              shiftLen,
	                              ht + h1,
	                              ht + h2,
	                              expectedHash,
	                              shiftedKey);
}

int CuckooHashTable::QueryLCP(uint64_t key, uint32_t& resultPos, uint32_t* allPositions)
{
	assert(m_hasCalledInit);
	
	__m128i h1, h2, h3, h4;
	uint64_t h5;
	XXH::XXHashArray(key, h1, h2, h3, h4, h5);
	
	__m128i hashModMask = _mm_set1_epi32(htMask);
	h1 = _mm_and_si128(h1, hashModMask);
	h2 = _mm_and_si128(h2, hashModMask);
	h4 = _mm_and_si128(h4, hashModMask);
	
	__m128i offset1, offset2, offset4;
	MultiplyBy3(h1, offset1);
	MultiplyBy3(h2, offset2);
	MultiplyBy3(h4, offset4);
	
	__m128i data1, data2, data4;
	data1 = _mm_i32gather_epi32(reinterpret_cast<int const*>(ht), offset1, 8);
	data2 = _mm_i32gather_epi32(reinterpret_cast<int const*>(ht), offset2, 8);
	data4 = _mm_i32gather_epi32(reinterpret_cast<int const*>(ht), offset4, 8);
	
	__m128i expect1 = _mm_and_si128(h3, HASH18_MASK);
	expect1 = _mm_or_si128(expect1, HASH_EXPECT_MASK1);
	
	h5 &= 0x3ffff0003ffffULL;
	h5 |= 0x8000000080000000ULL | (3ULL << 59) | (2ULL << 27);
	__m128i expect2 = _mm_set_epi64x(h5, h5);
	
	// TODO: 
	// experiment with the performance impact of early exit
	//
	data1 = _mm_and_si128(data1, HASH_EXPECT_MASK2);
	data2 = _mm_and_si128(data2, HASH_EXPECT_MASK2);
	data4 = _mm_and_si128(data4, HASH_EXPECT_MASK2);
	
	__m128i cmp1 = _mm_cmpeq_epi32(data1, expect1);
	__m128i cmp2 = _mm_cmpeq_epi32(data2, expect1);
	__m128i cmp3 = _mm_cmpeq_epi32(data4, expect2);
	
	int msk1 = _mm_movemask_ps(_mm_castsi128_ps(cmp1));
	int msk2 = _mm_movemask_ps(_mm_castsi128_ps(cmp2));
	int msk3 = _mm_movemask_ps(_mm_castsi128_ps(cmp3));
	
	msk1 = (msk1 << 2) | (msk3 >> 2);
	msk2 = (msk2 << 2) | (msk3 & 3);
	
	if (unlikely(msk1 & msk2)) goto _slowpath;
	
	{
		int msk = msk1 | msk2;
		if (msk == 0) 
		{ 
#ifdef ENABLE_STATS
			stats.m_lcpResultHistogram[2]++;
#endif
			return 2;	
		}
		
		int ordinal = 32 - __builtin_clz(msk);
		
		assert(1 <= ordinal && ordinal <= 6);
		
		// TODO: 
		// here we are selecting out the match from the SIMD register
		// alternate choice is to re-compute the hash value directly
		// investigate if re-computation is more performant
		//
		
		cmp1 = _mm_and_si128(cmp1, h1);
		cmp2 = _mm_and_si128(cmp2, h2);
		cmp3 = _mm_and_si128(cmp3, h4);
		
		_mm_storeu_si128(reinterpret_cast<__m128i*>(allPositions), cmp3);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(allPositions+4), _mm_or_si128(cmp1, cmp2));
		uint64_t* ptr = reinterpret_cast<uint64_t*>(allPositions);
		ptr[1] |= ptr[0];
		uint32_t pos = allPositions[ordinal+1];
		
		int ilen = ordinal + 2;
		int shiftLen = 64 - 8 * ilen;

#ifndef NDEBUG
		uint32_t hash18bit = XXH::XXHashFn3(key, ilen);
		hash18bit = hash18bit & ((1<<18) - 1);
		uint32_t expectedHash = hash18bit | ((ilen-1) << 27) | 0x80000000U;
		assert((ht[pos].hash & 0xf803ffffU) == expectedHash);
#endif

		if (unlikely((ht[pos].minKey >> shiftLen) != (key >> shiftLen))) goto _slowpath;
			
#ifdef ENABLE_STATS
		stats.m_lcpResultHistogram[ht[pos].GetIndexKeyLen()]++;
#endif

		resultPos = pos;
		uint64_t xorValue = key ^ ht[pos].minKey;
		if (!xorValue) return 8;
		
		int z = __builtin_clzll(xorValue);
		return z / 8;
	}
	
	// slow path handling hash conflict
	//
_slowpath:
	{
#ifdef ENABLE_STATS
		stats.m_slowpathCount++;
#endif
		memset(allPositions, 0, 32);
		uint64_t _buffer[6];
		uint32_t* buffer = reinterpret_cast<uint32_t*>(_buffer);
		// h1(5), h1(6), h1(7), h1(8)
		// h2(5), h2(6), h2(7), h2(8)
		// h2(3), h2(4), h1(3), h1(4)
		//
		_mm_storeu_si128(reinterpret_cast<__m128i*>(buffer), h1);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(buffer+4), h2);
		_mm_storeu_si128(reinterpret_cast<__m128i*>(buffer+8), h4);
			
		uint32_t pos = -1;
		bool found = false;
		if (ht[buffer[8]].IsEqualNoHash(key, 3)) { found = true; pos = buffer[8]; allPositions[2] = buffer[8]; }
		if (ht[buffer[10]].IsEqualNoHash(key, 3)) { found = true; pos = buffer[10]; allPositions[2] = buffer[10]; }
		if (ht[buffer[9]].IsEqualNoHash(key, 4)) { found = true; pos = buffer[9]; allPositions[3] = buffer[9]; }
		if (ht[buffer[11]].IsEqualNoHash(key, 4)) { found = true; pos = buffer[11]; allPositions[3] = buffer[11]; }
		if (ht[buffer[0]].IsEqualNoHash(key, 5)) { found = true; pos = buffer[0]; allPositions[4] = buffer[0]; }
		if (ht[buffer[4]].IsEqualNoHash(key, 5)) { found = true; pos = buffer[4]; allPositions[4] = buffer[4]; }
		if (ht[buffer[1]].IsEqualNoHash(key, 6)) { found = true; pos = buffer[1]; allPositions[5] = buffer[1]; }
		if (ht[buffer[5]].IsEqualNoHash(key, 6)) { found = true; pos = buffer[5]; allPositions[5] = buffer[5]; }
		if (ht[buffer[2]].IsEqualNoHash(key, 7)) { found = true; pos = buffer[2]; allPositions[6] = buffer[2]; }
		if (ht[buffer[6]].IsEqualNoHash(key, 7)) { found = true; pos = buffer[6]; allPositions[6] = buffer[6]; }
		if (ht[buffer[3]].IsEqualNoHash(key, 8)) { found = true; pos = buffer[3]; allPositions[7] = buffer[3] ; }
		if (ht[buffer[7]].IsEqualNoHash(key, 8)) { found = true; pos = buffer[7]; allPositions[7] = buffer[7]; }
		if (!found) 
		{ 
#ifdef ENABLE_STATS
			stats.m_lcpResultHistogram[2]++;
#endif
			return 2;	
		}

		assert(pos != -1);	// we will never have such a large hash table in debug..
		
#ifdef ENABLE_STATS
		stats.m_lcpResultHistogram[ht[pos].GetIndexKeyLen()]++;
#endif

		resultPos = pos;
		uint64_t xorValue = key ^ ht[pos].minKey;
		if (!xorValue) return 8;
			
		int z = __builtin_clzll(xorValue);
		return z / 8;
	}
}

void CuckooHashTable::HashTableCuckooDisplacement(uint32_t victimPosition, int rounds, bool& failed)
{
	if (rounds > 1000)
	{
		failed = true;
		return;
	}

	assert(ht[victimPosition].IsOccupied());
	if (likely(ht[victimPosition].IsNode()))
	{
		int ilen = ht[victimPosition].GetIndexKeyLen();
		uint64_t ikey = ht[victimPosition].GetIndexKey();
		
		uint32_t h1, h2;
		h1 = XXH::XXHashFn1(ikey, ilen) & htMask;
		h2 = XXH::XXHashFn2(ikey, ilen) & htMask;
		
		if (h1 == victimPosition)
		{
			swap(h1, h2);
		}
		assert(h2 == victimPosition);
		if (ht[h1].IsOccupied())
		{
			HashTableCuckooDisplacement(h1, rounds+1, failed);
			if (failed) return;
		}
		assert(!ht[h1].IsOccupied());
#ifdef ENABLE_STATS
		stats.m_movedNodesCount++;
#endif
		ht[victimPosition].MoveNode(&ht[h1]);
	}
	else
	{
		CuckooHashTableNode* owner = nullptr;
		rep(i, -3, 3)
		{
			CuckooHashTableNode* target = &ht[victimPosition + i];
			if (target->IsOccupiedAndNode() && !target->IsUsingInternalChildMap() && !target->IsExternalPointerBitMap())
			{
				int offset = ((target->hash >> 21) & 7) - 4;
				if (offset + i == 0)
				{
					owner = target;
					break;
				}
			}
		}
		assert(owner != nullptr);
#ifdef ENABLE_STATS
		stats.m_relocatedBitmapsCount++;
#endif
		owner->RelocateBitMap();
	}
	assert(!ht[victimPosition].IsOccupied());
}

MlpSet::MlpSet() 
	: m_memoryPtr(nullptr)
	, m_allocatedSize(-1)
	, m_hashTable()
#ifndef NDEBUG
	, m_hasCalledInit(false)
#endif
{ }
	
MlpSet::~MlpSet()
{
	if (m_memoryPtr != nullptr && m_memoryPtr != MAP_FAILED)
	{
		int ret = SAFE_HUGETLB_MUNMAP(m_memoryPtr, m_allocatedSize);
		assert(ret == 0);
		m_memoryPtr = nullptr;
	}
}
	
#ifdef ENABLE_STATS
MlpSet::Stats::Stats()
{
	memset(m_lowerBoundParentPathStepsHistogram, 0, sizeof m_lowerBoundParentPathStepsHistogram);
}
		
void MlpSet::Stats::ClearStats()
{
	memset(m_lowerBoundParentPathStepsHistogram, 0, sizeof m_lowerBoundParentPathStepsHistogram);
}
		
void MlpSet::Stats::ReportStats()	
{
	printf("MlpSet stats:\n");
	bool containsUsefulInfo = false;
	rep(i, 0, 7) 
	{
		if (m_lowerBoundParentPathStepsHistogram[i]) 
		{
			containsUsefulInfo = true;
		}
	}
	if (containsUsefulInfo)
	{
		printf("\tLower_bound queries parent-path walk length histogram:\n");
		rep(i, 0, 7)
		{
			printf("\tlen = %d: %u\n", i, m_lowerBoundParentPathStepsHistogram[i]);
		}
	}
}

void MlpSet::ClearStats()
{
	stats.ClearStats();
	m_hashTable.stats.ClearStats();
}
		
void MlpSet::ReportStats()
{
	stats.ReportStats();
	m_hashTable.stats.ReportStats();
}
#endif

void MlpSet::Init(uint32_t maxSetSize)
{
	assert(!m_hasCalledInit);
#ifndef NDEBUG
	m_hasCalledInit = true;
#endif
	// We are using _mm_i32gather_epi32 currently, which allows us to only index as far as 32GB memory
	// This is currently limiting how many elements we can hold in the container
	// If we use _mm256_i64gather_epi32 instead, we will support a max size of 2^30 
	// (the current 32-bit hash functiion becomes the limiting factor in this case)
	// In theory, we can support up to 2^37 elements (which should be sufficient for any in-memory workload), 
	// by making use of the currently unused 14 higher bits of HashFn3 to get two 39-bit hash value for Cuckoo
	// But for now let's just target 1<<28 input size
	// TODO: investigate what is the perf effect of changing _mm_i32gather_epi32 to _mm256_i64gather_epi32
	//
	ReleaseAssert(maxSetSize <= (1 << 28));
	maxSetSize = max(maxSetSize, 4096U);
	
	// compute how much memory to allocate
	// First, the top 3 levels of the tree
	//
	uint64_t sz = 32 + 8192 + 2 * 1024 * 1024;
	// Then, we need 6 HashTableNode's gap for internal bitmap
	//
	sz += sizeof(CuckooHashTableNode) * 6;
	// Pad sz to 128 bytes so the real hash table starts at 128-byte boundary
	//
	sz = RoundUpToNearestMultipleOf(sz, 128);
	uint64_t hashTableOffset = sz;
	// Real hash table size
	//
	uint64_t htSize = RoundUpToNearestPowerOf2(maxSetSize) * 4;
	// We need 6 slots gap in the end for internal bitmap as well
	//
	sz += (htSize + 6) * sizeof(CuckooHashTableNode);
	
	m_memoryPtr = mmap(NULL, 
	                   sz, 
	                   PROT_READ | PROT_WRITE, 
	                   MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, 
	                   -1 /*fd*/, 
	                   0 /*offset*/);
	ReleaseAssert(m_memoryPtr != MAP_FAILED);
	m_allocatedSize = sz;
		
	uintptr_t ptr = reinterpret_cast<uintptr_t>(m_memoryPtr);
	m_root = reinterpret_cast<uint64_t*>(ptr);
	m_treeDepth1 = reinterpret_cast<uint64_t*>(ptr + 32);
	m_treeDepth2 = reinterpret_cast<uint64_t*>(ptr + 32 + 8192);
	m_hashTable.Init(reinterpret_cast<CuckooHashTableNode*>(ptr + hashTableOffset), htSize - 1);
	
	memset(m_memoryPtr, 0, m_allocatedSize);
}

bool MlpSet::Insert(uint64_t value)
{
	assert(m_hasCalledInit);
	int lcpLen;
	// Handle LCP < 2 case first
	// This is supposed to be a L1 hit (working set 8KB)
	//
	{
		uint64_t h16bits = value >> 48;
		if (unlikely((m_treeDepth1[h16bits / 64] & (uint64_t(1) << (h16bits % 64))) == 0))
		{
			lcpLen = 2;
			m_root[(h16bits >> 8) / 64] |= uint64_t(1) << ((h16bits >> 8) % 64);
			m_treeDepth1[h16bits / 64] |= uint64_t(1) << (h16bits % 64);
			goto _end;
		}
	}
	
	// Issue the prefetch in case LCP turns out to be 2
	//
	MEM_PREFETCH(m_treeDepth2[(value >> 40) / 64]);
	
	// Since we know the LCP is >= 2, QueryLCP is applicable
	// 
	{
		uint32_t pos;
		uint64_t _allPositions[4];
		uint32_t* allPositions = reinterpret_cast<uint32_t*>(_allPositions);
		lcpLen = m_hashTable.QueryLCP(value, pos, allPositions);
		if (lcpLen == 8)
		{
			return false;
		}
		if (lcpLen > 2)
		{
			bool minKeyUpdated = false;
			int ilen = m_hashTable.ht[pos].GetIndexKeyLen();
			assert(ilen <= lcpLen && lcpLen <= m_hashTable.ht[pos].GetFullKeyLen());
			// Split as needed
			// Determine whether the path-compression string completely matched
			//
			if (lcpLen == m_hashTable.ht[pos].GetFullKeyLen())
			{
				// path-compression string matched, no need to split
				//
				m_hashTable.ht[pos].AddChild((value >> (56 - lcpLen * 8)) % 256);
				if (value < m_hashTable.ht[pos].minKey)
				{
					minKeyUpdated = true;
					m_hashTable.ht[pos].minKey = value;
				}
			}
			else
			{
				// need to split at unmatch point
				// (1) Add a new node with indexLen = lcpLen+1, fullKeyLen = ht[pos]'s fullKeyLen,
				//     childList = ht[pos]'s childList, this is the original subtree
				// (2) Modify the fullKeyLen of ht[pos] to lcpLen, and childList to have 
				//     only two children (corresponding byte of ht[pos].minKey and value)
				//     Now ht[pos] becomes the splitting point
				//
				uint64_t minKey = m_hashTable.ht[pos].minKey;
				uint32_t oldHash18bit = m_hashTable.ht[pos].GetHash18bit();
#ifndef NDEBUG
				vector<int> oldChildList = m_hashTable.ht[pos].GetAllChildren();
				int oldFullKeyLen = m_hashTable.ht[pos].GetFullKeyLen();
#endif

				// Add new node
				//
				{
					bool exist, failed;
					uint32_t newHash18bit = XXH::XXHashFn3(minKey, lcpLen + 1);
					newHash18bit = newHash18bit & ((1<<18) - 1);
					uint32_t x = m_hashTable.ReservePositionForInsert(lcpLen + 1 /*indexLen*/, 
						                                              minKey /*key*/,
						                                              newHash18bit /*hash18bit*/, 
						                                              exist /*out*/, 
						                                              failed /*out*/);
					assert(!exist && !failed);
					assert(!m_hashTable.ht[x].IsOccupied());
					m_hashTable.ht[pos].MoveNode(&(m_hashTable.ht[x]));
					m_hashTable.ht[x].AlterIndexKeyLen(lcpLen + 1);
					m_hashTable.ht[x].AlterHash18bit(newHash18bit);
				}
				
				// Re-construct ht[pos] (it has been cleared in MoveNode)
				//
				{
					assert(!m_hashTable.ht[pos].IsOccupied());
					uint64_t z = minKey;
					if (value < minKey)
					{
						z = value;
						minKeyUpdated = true;
					}
					m_hashTable.ht[pos].Init(ilen /*indexLen*/,
						                     lcpLen /*fullKeyLen*/,
						                     z /*minKey*/,
						                     oldHash18bit /*hash18bit*/,
						                     (minKey >> (56 - 8 * lcpLen)) % 256 /*firstChild*/);
					m_hashTable.ht[pos].AddChild((value >> (56 - 8 * lcpLen)) % 256);
				}  

#ifndef NDEBUG
				// Sanity check newly added nodes
				//
				{
					// Sanity check splitting node
					//
					bool found;
					uint32_t x = m_hashTable.Lookup(ilen, value, found);
					assert(found);
					assert(m_hashTable.ht[x].GetIndexKeyLen() == ilen);
					assert(m_hashTable.ht[x].GetFullKeyLen() == lcpLen);
					assert(m_hashTable.ht[x].minKey == min(value, minKey));
					vector<int> ch = m_hashTable.ht[x].GetAllChildren();
					assert(ch.size() == 2);
					int expectedChild1 = (value >> (56 - lcpLen * 8)) % 256;
					int expectedChild2 = (minKey >> (56 - lcpLen * 8)) % 256;
					if (expectedChild1 > expectedChild2) { swap(expectedChild1, expectedChild2); }
					assert(ch[0] == expectedChild1);
					assert(ch[1] == expectedChild2);
				}
				{
					// Sanity check original subtree
					//
					bool found;
					uint32_t x = m_hashTable.Lookup(lcpLen + 1, minKey, found);
					assert(found);
					assert(m_hashTable.ht[x].GetIndexKeyLen() == lcpLen + 1);
					assert(m_hashTable.ht[x].GetFullKeyLen() == oldFullKeyLen);
					assert(m_hashTable.ht[x].minKey == minKey);
					vector<int> ch = m_hashTable.ht[x].GetAllChildren();
					assert(ch.size() == oldChildList.size());
					rep(i, 0, int(ch.size()) - 1)
					{
						assert(ch[i] == oldChildList[i]);
					}
				}
#endif      
			}
			// Update minKey along the parent path
			//
			if (minKeyUpdated)
			{
				for (ilen--; ilen > 2; ilen--)
				{
					uint32_t pos = allPositions[ilen - 1];
#ifndef NDEBUG
					if (pos != 0)
					{
						uint32_t hash18bit = XXH::XXHashFn3(value, ilen);
						hash18bit = hash18bit & ((1<<18) - 1);
						uint32_t expectedHash = hash18bit | ((ilen-1) << 27) | 0x80000000U;
						assert((m_hashTable.ht[pos].hash & 0xf803ffffU) == expectedHash);
					}
#endif
					int shiftLen = 64 - 8 * ilen;
					if (m_hashTable.ht[pos].IsOccupiedAndNode() && (m_hashTable.ht[pos].minKey >> shiftLen) == (value >> shiftLen))
					{
						assert(m_hashTable.ht[pos].GetIndexKeyLen() == ilen);
						if (value < m_hashTable.ht[pos].minKey)
						{
							m_hashTable.ht[pos].minKey = value;
						}
						else
						{
							break;
						}
					}
				}
			}
		}
	}
	
_end:
	// Now we know the true LCP and the tree has been setup with correct splitting, insert node
	//
	{
		bool exist, failed;
		m_hashTable.Insert(lcpLen + 1 /*indexLen*/,
		                   8 /*fullKeyLen*/,
		                   value /*minKey*/, 
		                   -1 /*firstChild*/,
		                   exist /*out*/, 
		                   failed /*out*/);
		assert(!exist && !failed);
	}
	
	// Finally, if lcp == 2, we need to set the corresponding m_treeDepth2 bit
	//
	if (lcpLen == 2)
	{
		assert((m_treeDepth2[(value >> 40) / 64] & (uint64_t(1) << ((value >> 40) % 64))) == 0);
		m_treeDepth2[(value >> 40) / 64] |= uint64_t(1) << ((value >> 40) % 64);
	}	
	return true;
}

bool MlpSet::Exist(uint64_t value)
{
	assert(m_hasCalledInit);
	uint32_t pos;
	uint64_t _allPositions[4];
	uint32_t* allPositions = reinterpret_cast<uint32_t*>(_allPositions);
	int lcpLen = m_hashTable.QueryLCP(value, pos, allPositions);
	return (lcpLen == 8);
}

MlpSet::Promise MlpSet::LowerBoundInternal(uint64_t value, bool& found)
{
	assert(m_hasCalledInit);
	found = true;
	
	// Issue the prefetch in case LCP turns out to be 2
	//
	MEM_PREFETCH(m_treeDepth2[(value >> 48) * 4]);
	
#ifdef ENABLE_STATS
	int numParentPathSteps = 0;
	Auto(
		assert(numParentPathSteps < 8);
		stats.m_lowerBoundParentPathStepsHistogram[numParentPathSteps]++;
	);
#endif

	uint32_t pos;
	uint64_t _allPositions[4];
	uint32_t* allPositions = reinterpret_cast<uint32_t*>(_allPositions);
	int lcpLen = m_hashTable.QueryLCP(value, pos, allPositions);
	if (lcpLen == 8)
	{
		return Promise(&m_hashTable.ht[pos]);
	}
	if (lcpLen == 2)
	{
		goto _flat_mapping;
	}
	
	// lcp in hash table
	//
	{
		int dlen = m_hashTable.ht[pos].GetFullKeyLen();
		if (dlen == lcpLen)
		{
			// path compression string matches, lower bound on child
			//
			uint32_t child = (value >> (56 - dlen * 8)) & 255;
			int lbChild = m_hashTable.ht[pos].LowerBoundChild(child);
			if (lbChild == -1) 
			{
				goto _parent;
			}
			assert(lbChild != child);
			// return the minimum value in lbChild subtree
			//
			uint64_t keyToFind = value & (~(255ULL << (56 - dlen * 8)));
			keyToFind |= uint64_t(lbChild) << (56 - dlen * 8);
			return m_hashTable.GetLookupMustExistPromise(dlen + 1, keyToFind);
		}
		else
		{
			// path compression string does not match
			// either the given value is smaller than the whole subtree, or larger than the whole subtree
			//
			if (value < m_hashTable.ht[pos].minKey)
			{
				// smaller than whole subtree, result is just subtreeMin
				//
				return Promise(&m_hashTable.ht[pos]);
			}
			else
			{	
				// larger than subtreeMax, need to visit parent path
				//
				goto _parent;
			}
		}
	}
	
_parent:
	// The specified value is larger than the maximum in the subtree
	// We need to return the smallest value larger than subtreeMax by visiting the parent path
	// 
	{
		int ilen = m_hashTable.ht[pos].GetIndexKeyLen() - 1;
		for (; ilen > 2; ilen--)
		{
#ifdef ENABLE_STATS
			numParentPathSteps++;
#endif
			uint32_t pos = allPositions[ilen - 1];
#ifndef NDEBUG
			if (pos != 0)
			{
				uint32_t hash18bit = XXH::XXHashFn3(value, ilen);
				hash18bit = hash18bit & ((1<<18) - 1);
				uint32_t expectedHash = hash18bit | ((ilen-1) << 27) | 0x80000000U;
				assert((m_hashTable.ht[pos].hash & 0xf803ffffU) == expectedHash);
			}
#endif
			int shiftLen = 64 - 8 * ilen;
			if (m_hashTable.ht[pos].IsOccupiedAndNode() && (m_hashTable.ht[pos].minKey >> shiftLen) == (value >> shiftLen))
			{
				assert(m_hashTable.ht[pos].GetIndexKeyLen() == ilen);
				int dlen = m_hashTable.ht[pos].GetFullKeyLen();
				assert((m_hashTable.ht[pos].minKey >> (64 - dlen * 8)) == (value >> (64 - dlen * 8)));
				uint32_t child = (value >> (56 - dlen * 8)) & 255;
				if (child < 255)
				{
					int lbChild = m_hashTable.ht[pos].LowerBoundChild(child + 1);
					if (lbChild != -1) 
					{
						assert(lbChild != child);
						// return the minimum value in lbChild subtree
						//
						uint64_t keyToFind = value & (~(255ULL << (56 - dlen * 8)));
						keyToFind |= uint64_t(lbChild) << (56 - dlen * 8);
						return m_hashTable.GetLookupMustExistPromise(dlen + 1, keyToFind);
					}
				}
			}
		}
	}
	
_flat_mapping:
	// We have reached lv2 of the tree, which are stored in the flat bitarray instead of the hash table
	//
#ifdef ENABLE_STATS
	numParentPathSteps++;
#endif
	uint64_t high24bits = value >> 40;
	if ((high24bits & 255) < 255)
	{
		int lv2LbChild = Bitmap256LowerBound(m_treeDepth2 + (high24bits >> 8) * 4, (high24bits & 255) + 1);
		if (lv2LbChild != -1)
		{
			uint64_t keyToFind = ((high24bits >> 8) << 48) | (uint64_t(lv2LbChild) << 40);
			return m_hashTable.GetLookupMustExistPromise(3, keyToFind);
		}
	}
	// check lv1 of tree
	//
#ifdef ENABLE_STATS
	numParentPathSteps++;
#endif
	if (((high24bits >> 8) & 255) < 255)
	{
		int lv1LbChild = Bitmap256LowerBound(m_treeDepth1 + (high24bits >> 16) * 4, ((high24bits >> 8) & 255) + 1);
		if (lv1LbChild != -1)
		{
			uint64_t high16bits = ((high24bits >> 16) << 8) | lv1LbChild;
			int lv2FirstChild = Bitmap256LowerBound(m_treeDepth2 + high16bits * 4, 0 /*child*/);
			assert(lv2FirstChild != -1);
			uint64_t keyToFind = (high16bits << 48) | (uint64_t(lv2FirstChild) << 40);
			return m_hashTable.GetLookupMustExistPromise(3, keyToFind);
		}
	}
	// finally check root
	//
#ifdef ENABLE_STATS
	numParentPathSteps++;
#endif
	if ((high24bits >> 16) < 255)
	{
		int lv0LbChild = Bitmap256LowerBound(m_root, (high24bits >> 16) + 1);
		if (lv0LbChild != -1)
		{
			int lv1FirstChild = Bitmap256LowerBound(m_treeDepth1 + lv0LbChild * 4, 0 /*child*/);
			assert(lv1FirstChild != -1);
			uint64_t high16bits = (lv0LbChild << 8) | lv1FirstChild;
			int lv2FirstChild = Bitmap256LowerBound(m_treeDepth2 + high16bits * 4, 0 /*child*/);
			assert(lv2FirstChild != -1);
			uint64_t keyToFind = (high16bits << 48) | (uint64_t(lv2FirstChild) << 40);
			return m_hashTable.GetLookupMustExistPromise(3, keyToFind);
		}
	}
	// not found
	//
	found = false;
	return Promise();
}

MlpSet::Promise MlpSet::LowerBound(uint64_t value)
{
	bool found;
	Promise p = LowerBoundInternal(value, found);
	if (found) 
	{
		p.Prefetch();
	}
	return p;
}

uint64_t MlpSet::LowerBound(uint64_t value, bool& found)
{
	Promise p = LowerBoundInternal(value, found);
	if (found) 
	{
		p.Prefetch();
		return p.Resolve();
	}
	else
	{
		return 0xffffffffffffffffULL;
	}
}

}	// namespace MlpSetUInt64

