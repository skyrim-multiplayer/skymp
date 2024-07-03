#pragma once

namespace espm {

class RecordHeader;
class CompressedFieldsCache;

}

float GetWeightFromRecord(const espm::RecordHeader* record,
                          espm::CompressedFieldsCache& cache);
