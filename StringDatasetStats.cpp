#include "common.h"

#include "gtest/gtest.h"

namespace {

TEST(Misc, EmailDatasetCount)
{
	set<string> s;
	FILE* file = fopen("emails-validated-random-only-30-characters.txt.sorted", "r");
	char buffer[200];
	int cnt = 0;
	while (fgets(buffer, 200, file) != nullptr)
	{
		int len = strlen(buffer);
		// remove newline
		buffer[len-1] = 0;
		s.insert(buffer);
		cnt++;
		if (cnt % 1000000 == 0) printf("%d\n", cnt);
	}
	fclose(file);
	file = fopen("emails_random.txt", "r");
	while (fgets(buffer, 200, file) != nullptr)
	{
		int len = strlen(buffer);
		// remove newline
		buffer[len-1] = 0;
		s.insert(buffer);
		cnt++;
		if (cnt % 1000000 == 0) printf("%d\n", cnt);
	}
	fclose(file);
	printf("%d\n", s.size());
}

TEST(Misc, EmailDatasetStats)
{
	int* stats = new int[200000000];
	memset(stats, 0, sizeof stats);
	map<string, set<string> > *s;
	s = new map<string, set<string> >();
	FILE* file = fopen("emails-validated-random-only-30-characters.txt.sorted", "r");
	int cnt = 0;
	char buffer[200];
	while (fgets(buffer, 200, file) != nullptr)
	{
		int len = strlen(buffer);
		// remove newline
		buffer[len-1] = 0;
		len--;
		
		rep(i, 0, 5)
		{
			if (i*6 >= len) continue;
		
			int k = min(6, len - i*6);
			char ch[200];
			memcpy(ch, buffer + i*6, k);
			ch[k] = 0;
			char pref[200];
			memcpy(pref, buffer, i*6);
			pref[i*6] = 0;
			if (!(*s)[pref].count(ch))
			{
				int x = (*s)[pref].size();
				stats[x]--;
				stats[x+1]++;
				(*s)[pref].insert(ch);
			}
		}
		cnt++;
		if (cnt % 1000000 == 0) { printf("%d\n", cnt); fflush(stdout); }
	}
	fclose(file);
	
	printf("Stats:\n");
	rep(i, 0, 200000000-1)
	{
		if (stats[i] > 0)
		{
			printf("%d: %d\n", i, stats[i]);
		}
	}
}

TEST(Misc, EmailFanoutResultMining)
{
	FILE* file = fopen("email_fanout.txt", "r");
	int sz, num;
	int cnt1 = 0, cnt2 = 0, cnt3 = 0, cnt4 = 0;
	uint64_t tot = 0;
	while (fscanf(file, "%d:%d", &sz, &num) == 2)
	{
		if (sz == 1) 
		{
			// do nothing
		}
		else if (sz <= 8)
		{
			cnt1+=num;
		}
		else if (sz <= 500)
		{
			cnt2+=num;
		}
		else if (sz <= 10000)
		{
			cnt3+=num;
		}
		else 
		{
			cnt4+=num;
		}	
		if (sz > 1)
		{
			tot += sz * num;
		}
	}
	printf("%d %d %d %d %lld\n", cnt1, cnt2, cnt3, cnt4, tot);
	fclose(file);
}

}	// annoymous namespace

