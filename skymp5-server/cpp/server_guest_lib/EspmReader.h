#include "Loader.h"
#include "espm.h"

class EspmReader
{
public:
  ~EspmReader() = default;
  static std::shared_ptr<EspmReader> GetEspmReader(
    espm::CompressedFieldsCache& espmCache,
    const espm::CombineBrowser& browser);

private:
  EspmReader(espm::CompressedFieldsCache& espmCache,
             const espm::CombineBrowser& browser);
  espm::CompressedFieldsCache& espmCache;
  const espm::CombineBrowser& browser;
};
