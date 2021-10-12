#include "EspmReader.h"

std::shared_ptr<EspmReader> EspmReader::GetEspmReader(
  espm::CompressedFieldsCache& espmCache, const espm::CombineBrowser& browser)
{
  return std::make_unique<EspmReader>(EspmReader(espmCache, browser));
}

EspmReader::EspmReader(espm::CompressedFieldsCache& espmCache,
                       const espm::CombineBrowser& browser)
  : espmCache(espmCache)
  , browser(browser)
{
}
