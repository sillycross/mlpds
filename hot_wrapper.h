#pragma once

#include "common.h"
#include "WorkloadInterface.h"

namespace HotTrieUInt64
{

template<bool enforcedDep>
void HotTrieExecuteWorkload(WorkloadUInt64& workload);

}

