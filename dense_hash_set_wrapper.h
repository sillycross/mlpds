#pragma once

#include "common.h"
#include "WorkloadInterface.h"

namespace DenseHashSetUInt64
{

template<bool enforcedDep>
void DenseHashSetExecuteWorkload(WorkloadUInt64& workload);

}

