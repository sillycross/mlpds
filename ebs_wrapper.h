#pragma once

#include "common.h"
#include "WorkloadInterface.h"

namespace EbsUInt64
{

template<bool enforcedDep>
void EbsExecuteWorkload(WorkloadUInt64& workload);

}

