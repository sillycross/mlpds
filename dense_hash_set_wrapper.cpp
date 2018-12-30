#include "dense_hash_set_wrapper.h"

#include "common.h"
// dense_hash_set is also using the macro rep
#undef rep

#include <sparsehash/dense_hash_set>

using google::dense_hash_set;

namespace DenseHashSetUInt64
{

// Allocator using huge pages
template <class T>
struct Mallocator {
  typedef T value_type;
  typedef size_t size_type;
  typedef ptrdiff_t difference_type;

  typedef T* pointer;
  typedef const T* const_pointer;
  typedef T& reference;
  typedef const T& const_reference;
  
  Mallocator() = default;
  template <class U> constexpr Mallocator(const Mallocator<U>& o) noexcept { }
  T* allocate(std::size_t n) 
  {
  	size_t len = n * sizeof(T);
  	printf("Requested allocation of %llu elements of size %llu\n", (unsigned long long)n, (unsigned long long)sizeof(T));
  	void* ptr = mmap(NULL, 
	                 len, 
	                 PROT_READ | PROT_WRITE, 
	                 MAP_PRIVATE | MAP_ANONYMOUS | MAP_HUGETLB, 
	                 -1 /*fd*/, 
	                 0 /*offset*/);
	ReleaseAssert(ptr != MAP_FAILED);
	memset(ptr, 0, len);
	T* p = reinterpret_cast<T*>(ptr);
    printf("Allocated at %llx\n", (unsigned long long)((uintptr_t)p));
    return p;
  }
  void deallocate(T* p, std::size_t) noexcept 
  { 
  	printf("Deallocation at ptr %llx, though it's no-op for us\n", (unsigned long long)((uintptr_t)p));
  }
  size_type max_size() const  {
    return static_cast<size_type>(-1) / sizeof(value_type);
  }
  template<class U>
  struct rebind {
    typedef Mallocator<U> other;
  };
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

uint32_t XXH32_CoreLogic(uint64_t key, int32_t seed, uint32_t multiplier)
{
	uint32_t low = key;
	uint32_t high = key >> 32;
	uint32_t h32 = PRIME32_5 + seed + 8;
	h32 ^= high * multiplier; 
	h32 = XXH_rotl32(h32, 17) * PRIME32_4;
	h32 ^= low * multiplier; 
	h32 = XXH_rotl32(h32, 17) * PRIME32_4;
	return XXH32_avalanche(h32);
}

uint32_t XXHashFn(uint64_t key)
{
	return XXH32_CoreLogic(key, 0 /*seed*/, PRIME32_3);
}

} // namespace XXH

struct HashFn
{
	uint64_t operator()(const uint64_t& v) const
	{
		return XXH::XXHashFn(v);
	}
};

typedef dense_hash_set<uint64_t, HashFn, equal_to<uint64_t>, Mallocator<uint64_t> > DenseHashSet;

// just to fool the compiler to not optimize out the iterator, so we can have a data dependency
// i don't really have idea why the compiler ignores the noinline direction
// unless i wrap it twice...
// This could also be achieved by simply adding "noinline" in the library's prototype
// but i don't want to modify the library
pair<DenseHashSet::iterator, bool> __attribute__((noinline)) insertWrapper(DenseHashSet& s, uint64_t key)
{
	pair<DenseHashSet::iterator, bool> r = s.insert(key);
	return r;
}

DenseHashSet::iterator __attribute__((noinline)) insertWrapperWrapper(DenseHashSet& s, uint64_t key)
{
	pair<DenseHashSet::iterator, bool> r = insertWrapper(s, key);
	return r.first;
}

template<bool enforcedDep>
void DenseHashSetExecuteWorkload(WorkloadUInt64& workload)
{
	printf("DenseHashSet executing workload, enforced dependency = %d\n", (enforcedDep ? 1 : 0));
	
	DenseHashSet s;
	s.set_empty_key(0xffffffffffffffffULL);
	s.max_load_factor(0.7);
	s.resize(workload.numInitialValues + 10000);
	
	printf("DenseHashSet populating initial values..\n");
	if (enforcedDep)
	{
		AutoTimer timer;
		uint64_t lastAnswer = 0;
		for (int i = 0; i < workload.numInitialValues; i++)
		{
			uint64_t realKey = workload.initialValues[i] ^ lastAnswer;
			DenseHashSet::iterator it = insertWrapperWrapper(s, realKey);
			lastAnswer = *it;
		}
	}
	else
	{
		AutoTimer timer;
		for (int i = 0; i < workload.numInitialValues; i++)
		{
			s.insert(workload.initialValues[i]);
		}
	}
	
	printf("DenseHashSet executing workload..\n");
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
					case WorkloadOperationType::INSERT:
					{
						uint64_t r = s.insert(realKey).second;
						answer = r;
						break;
					}
					case WorkloadOperationType::EXIST:
					{
						auto it = s.find(realKey);
						uint64_t r = (it != s.end()) ? (*it) : 0;
						answer = r;
						break;
					}
				}
				workload.results[i] = answer;
				lastAnswer = answer;
			}
		}
		else
		{
			for (int i = 0; i < workload.numOperations; i++)
			{
				uint64_t answer;
				uint64_t realKey = workload.operations[i].key;
				switch (workload.operations[i].type)
				{
					case WorkloadOperationType::INSERT:
					{
						bool r = s.insert(realKey).second;
						answer = r;
						break;
					}
					case WorkloadOperationType::EXIST:
					{
						bool r = (s.find(realKey) != s.end());
						answer = r;
						break;
					}
				}
				workload.results[i] = answer;
			}
		}
	}
	
	printf("DenseHashSet workload completed.\n");
}

// explicitly instantiate template function
//
template void DenseHashSetExecuteWorkload<false>(WorkloadUInt64& workload);
template void DenseHashSetExecuteWorkload<true>(WorkloadUInt64& workload);

}	// DenseHashSetUInt64

