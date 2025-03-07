#pragma once

#include "antigo/Context.h"

namespace Antigo {

struct ExecutionData;
ExecutionData& GetCurrentExecutionData();

bool HasExceptionWitness();
ResolvedContext PopExceptionWitness();

bool HasExceptionWitnessOrphan();
ResolvedContext PopExceptionWitnessOrphan();

}
