
#include "savefile/SFSeekerOfDifferences.h"
#include "savefile/SFChangeFormNPC.h"

#include <bitset>
#include <iostream>

#include <zlib.h>

namespace {
// Example program

void ReportFlags(uint32_t v, std::ostream& out) noexcept
{
  auto bits = std::bitset<32>(v);
  auto s = bits.to_string();

  for (int i = 0; i < 3; ++i) {
    auto p = s.rfind(' ') + 1;
    s.insert(s.begin() + 8 + p, ' ');
  }
  out << s << "\n";
}
}

void SaveFile_::SeekerOfDifferences::ZlibDecompress(const uint8_t* in,
                                                    size_t inSize,
                                                    uint8_t* out,
                                                    size_t outSize)
{
  z_stream infstream;
  infstream.zalloc = Z_NULL;
  infstream.zfree = Z_NULL;
  infstream.opaque = Z_NULL;

  infstream.avail_in = inSize;
  infstream.next_in = const_cast<uint8_t*>(in);
  infstream.avail_out = outSize;
  infstream.next_out = out;

  inflateInit(&infstream);

  int res = inflate(&infstream, Z_NO_FLUSH);
  if (res < Z_OK)
    throw std::runtime_error("inflate() failed with code " +
                             std::to_string(res));
  res = inflateEnd(&infstream);
  if (res < Z_OK)
    throw std::runtime_error("inflateEnd() failed with code " +
                             std::to_string(res));
}

size_t SaveFile_::SeekerOfDifferences::ZlibCompress(const uint8_t* in,
                                                    size_t inSize,
                                                    uint8_t* out,
                                                    size_t outMaxSize)
{
  z_stream defstream;
  defstream.zalloc = Z_NULL;
  defstream.zfree = Z_NULL;
  defstream.opaque = Z_NULL;

  defstream.avail_in = inSize;
  defstream.next_in = const_cast<uint8_t*>(in);
  defstream.avail_out = outMaxSize;
  defstream.next_out = out;

  // the actual compression work.
  deflateInit(&defstream, Z_BEST_COMPRESSION);
  int res = deflate(&defstream, Z_FINISH);

  const auto outputSize = defstream.next_out - (uint8_t*)out;
  res = deflateEnd(&defstream);

  return outputSize;
}

SaveFile_::SeekerOfDifferences::ComparisonDifferences
SaveFile_::SeekerOfDifferences::StartCompare()
{

  auto& changeFormsFirstObject = firstObject->changeForms;
  auto& changeFormsSecondObject = secondObject->changeForms;

  ComparisonDifferences compareResult;

  for (auto& formObj1 : changeFormsFirstObject) {
    if (!formObj1.Is_ACHR_Type())
      continue;

    if (!formObj1.formID.IsPlayerID())
      continue;

    for (auto& formObj2 : changeFormsSecondObject) {

      if (formObj1.type != formObj2.type)
        continue;
      if (formObj1.formID != formObj2.formID)
        continue;

      /*firstObject->fileLocationTable.formIDArrayCountOffset -=
      formObj1.data.size();
      firstObject->fileLocationTable.formIDArrayCountOffset +=
      formObj2.data.size();

      firstObject->fileLocationTable.unknownTable3Offset -=
      formObj1.data.size(); firstObject->fileLocationTable.unknownTable3Offset
      += formObj2.data.size();

      firstObject->fileLocationTable.globalDataTable3Offset -=
      formObj1.data.size();
      firstObject->fileLocationTable.globalDataTable3Offset +=
      formObj2.data.size();

      formObj1.changeFlags = formObj2.changeFlags;
      formObj1.data = formObj2.data;
      formObj1.length1 = formObj2.length1;
      formObj1.length2 = formObj2.length2;
      formObj1.version = formObj2.version;*/

      /*std::array<Data, 2> result;
      std::array<ChangeForm*, 2> formObjs = { &formObj1, &formObj2 };

      for (int i = 0; i < 2; ++i) {
              result[i].changeFlags = formObjs[i]->changeFlags;
              result[i].value = formObjs[i]->data;

                      ReportFlags(formObjs[i]->changeFlags, std::cout);

              if (formObjs[i]->length2 != 0) {
                      std::vector<uint8_t> res;
                      res.resize(formObjs[i]->length2);
                      ZlibDecompress(formObjs[i]->data.data(),
      formObjs[i]->length1, res.data(), res.size());

                      result[i].value = res;
              }
              else {
                      result[i].value = formObjs[i]->data;
              }
      }

      compareResult.push_back(result);*/
      break;
    }
  }
  return compareResult;
}

void SaveFile_::SeekerOfDifferences::CoutVector(std::vector<uint8_t> vector,
                                                std::string nameObject)
{

  std::cout << nameObject << " be kept " << vector.size() << " bytes."
            << std::endl;

  for (auto& item : vector)
    Write(item);
}
