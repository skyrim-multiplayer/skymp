#pragma once
#include "libespm/CombineBrowser.h"
#include "libespm/LookupResult.h"
#include "libespm/REFR.h"
#include <cstdint>

class NiPoint3;

namespace LocationalDataUtils {
const NiPoint3& GetPos(const espm::REFR::LocationalData* locationalData);
NiPoint3 GetRot(const espm::REFR::LocationalData* locationalData);
uint32_t GetWorldOrCell(const espm::CombineBrowser& br,
                        const espm::LookupResult& refrLookupRes);
}
