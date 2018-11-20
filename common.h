#pragma once

#include <cstdlib>
#include <cstdio>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <vector>
#include <set>
#include <map>
#include <cstring>
#include <cassert>
#include <queue>
#include <x86intrin.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <err.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <immintrin.h>

#include "fasttime.h"

using namespace std;

typedef long long LL;
typedef unsigned long long ULL;

#define rep(i,l,r) for (int i=(l); i<=(r); i++)
#define repd(i,r,l) for (int i=(r); i>=(l); i--)
#define rept(i,c) for (__typeof((c).begin()) i=(c).begin(); i!=(c).end(); i++)

#define MEM_PREFETCH(x) _mm_prefetch((char*)(&(x)),_MM_HINT_T0);

#define PTRRAW(x) ((__typeof(x))(((uintptr_t)(x))&(~7)))
#define PTRTAG(x) (((uintptr_t)(x))&7)
#define PTRSET(x, b) ((__typeof(x))((((uintptr_t)(x))&(~7))|(b)))

#define likely(expr)     __builtin_expect((expr) != 0, 1)
#define unlikely(expr)   __builtin_expect((expr) != 0, 0)

#define TOKEN_PASTEx(x, y) x ## y
#define TOKEN_PASTE(x, y) TOKEN_PASTEx(x, y)

#define ALWAYS_INLINE __attribute__((always_inline))

struct AutoTimer
{
	double *m_result;
	fasttime_t m_start;
	AutoTimer(): m_result(nullptr), m_start(gettime()) {}
	AutoTimer(double *result): m_result(result), m_start(gettime()) {}
	~AutoTimer()
	{
		fasttime_t m_end = gettime();
		double timeElapsed = tdiff(m_start, m_end);
		if (m_result != nullptr) *m_result = timeElapsed;
		fprintf(stderr, "AutoTimer: %.6lf second elapsed.\n", timeElapsed);
	}
};

template<typename T>
class AutoOutOfScope	
{
public:
   AutoOutOfScope(T& destructor) : m_destructor(destructor) { }
   ~AutoOutOfScope() { m_destructor(); }
private:
   T& m_destructor;
};
	
#define Auto_INTERNAL(Destructor, counter) \
    auto TOKEN_PASTE(auto_func_, counter) = [&]() { Destructor; }; \
    AutoOutOfScope<decltype(TOKEN_PASTE(auto_func_, counter))> TOKEN_PASTE(auto_, counter)(TOKEN_PASTE(auto_func_, counter));
	
#define Auto(Destructor) Auto_INTERNAL(Destructor, __COUNTER__)

struct ReleaseAssertFailure
{
	static void Fire(const char *__assertion, const char *__file,
	                 unsigned int __line, const char *__function)
	{
		printf("%s:%u: %s: Assertion `%s' failed.\n", __file, __line, __function, __assertion);
		exit(0);
	}
};

#define ReleaseAssert(expr)							\
     (static_cast <bool> (expr)						\
      ? void (0)							        \
      : ReleaseAssertFailure::Fire(#expr, __FILE__, __LINE__, __extension__ __PRETTY_FUNCTION__))

