#include <cstdio>
#include <cstdlib> 
#include "gtest/gtest.h"

namespace {

void PrintInformation()
{
#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
	printf("--------------- General Information ---------------\n");
	printf("%-25s", "Host:"); fflush(stdout);
	std::ignore = system("whoami | tr -d '\\n' && printf '@' && cat /etc/hostname");
	printf("%-25s%s\n", "Build flavor:", TOSTRING(BUILD_FLAVOR));
	printf("%-25s", "HugePage size:"); fflush(stdout);
	std::ignore = system("cat /proc/meminfo | grep Hugepagesize | tr -s ' ' | cut -d' ' -f 2,3");
	printf("%-25s", "# total HugePages:"); fflush(stdout);
	std::ignore = system("cat /proc/meminfo | grep HugePages_Total | tr -s ' ' | cut -d' ' -f 2");
	printf("%-25s", "# free HugePages:"); fflush(stdout);
	std::ignore = system("cat /proc/meminfo | grep HugePages_Free | tr -s ' ' | cut -d' ' -f 2");
	printf("---------------------------------------------------\n");
#undef TOSTRING
#undef STRINGIFY
}

}	// annoymous namespace

int main(int argc, char **argv)
{
	PrintInformation();
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

