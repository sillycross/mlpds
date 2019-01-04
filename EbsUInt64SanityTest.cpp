#include "gtest/gtest.h"

#include "common.h"

#include "btree_array.h"

namespace {

using namespace fbs;

typedef btree_array<64/sizeof(uint64_t),uint64_t,uint64_t,true> Array;

TEST(EbsUInt64Test, Sanity) 
{
	int n = 10000;
	uint64_t* data = new uint64_t[n];
	ReleaseAssert(data != nullptr);
	Auto(delete [] data);
	
	rep(i, 0, n-1) 
	{
		uint64_t x = 0;
		rep(j, 0, 7) x = x * 256 + rand() % 255;
		data[i] = x;
	}
	
	int numQueries = 10000;
	uint64_t* queries = new uint64_t[numQueries];
	ReleaseAssert(queries != nullptr);
	Auto(delete [] queries);
	
	rep(i, 0, numQueries-1) 
	{
		uint64_t x = 0;
		rep(j, 0, 7) x = x * 256 + rand() % 255;
		queries[i] = x;
	}
	
	uint64_t* expected = new uint64_t[numQueries];
	ReleaseAssert(expected != nullptr);
	Auto(delete [] expected);
	
	uint64_t* actual = new uint64_t[numQueries];
	ReleaseAssert(actual != nullptr);
	Auto(delete [] actual);
	memset(actual, 0, sizeof(uint64_t)*numQueries);
	
	{	
		set<uint64_t> s;
		rep(i, 0, n-1) s.insert(data[i]);
		rep(i, 0, numQueries-1) 
		{
			auto it = s.lower_bound(queries[i]);
			if (it == s.end())
				expected[i] = 0xffffffffffffffffULL;
			else
				expected[i] = *it;
		}
	}
	
	sort(data, data+n);
	
	printf("Data gen ok\n");
	
	Array A(data, n);
	
	rep(i, 0, numQueries-1)
	{
		int x = A.search(queries[i]);
		if (x < n)
			actual[i] = A.get_data(x);
		else
			actual[i] = 0xffffffffffffffffULL;
		ReleaseAssert(actual[i] == expected[i]);
	}
	printf("ok\n");
}

}	// annoymous namespace
