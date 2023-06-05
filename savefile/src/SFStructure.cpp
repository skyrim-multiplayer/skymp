#include "savefile/SFStructure.h"

#include <cstring>
#include <stdexcept>

SaveFile_::RefID SaveFile_::RefID::CreateRefId(SaveFile& parentSaveFile,
                                               uint32_t formId)
{
  RefID res;

  const auto countWas = parentSaveFile.formIDArrayCount;
  const size_t n = countWas + 1;
  uint32_t* newFormIDArray = new uint32_t[n];

  memcpy(newFormIDArray, parentSaveFile.formIDArray.data(), countWas);
  newFormIDArray[countWas] = formId;

  parentSaveFile.formIDArray = { newFormIDArray, newFormIDArray + n };
  parentSaveFile.formIDArrayCount = countWas + 1;

  // fix offset
  parentSaveFile.fileLocationTable.unknownTable3Offset += 4;

  // 255 => 00 00 FF
  // 256 => 00 01 00
  // 65536 => error
  const auto index =
    countWas + 1; // as uesp.net says, formIDArray index starts in 1
  if (index >= 65536)
    throw std::runtime_error("too many elements was in FormIDArray (" +
                             std::to_string(countWas) + ")");
  res.byte0 = 0;
  res.byte1 = (index / 256) % 256;
  res.byte2 = index % 256;

  return res;
}

SaveFile_::ChangeForm* SaveFile_::SaveFile::GetChangeFormByRefID(
  SaveFile_::RefID refID, const uint8_t& type)
{
  for (auto& form : this->changeForms) {
    if ((form.type & 0b00111111) == type &&
        form.formID == refID) /// Upper 2 bits represent the size of the data
                              /// lengths: zero them
      return &form;
  }
  return nullptr;
}

SaveFile_::GlobalVariables::GlobalVariable*
SaveFile_::SaveFile::GetGlobalvariableByRefID(SaveFile_::RefID& refID)
{
  GlobalData& gData = this->globalDataTable1[GLOBAL_VARIABLES_INDEX];

  if (gData.type != GLOBAL_VARIABLES_INDEX)
    return nullptr;

  GlobalVariables* globalsVar =
    reinterpret_cast<GlobalVariables*>(gData.data.get());

  if (!globalsVar)
    return nullptr;

  for (auto& gVar : globalsVar->globals) {
    if (gVar.formID == refID) {
      return &gVar;
    }
  }
  return nullptr;
}

int64_t SaveFile_::SaveFile::FindIndexInFormIdArray(uint32_t refID)
{
  for (uint32_t i = 0; i < this->formIDArray.size(); ++i) {
    if (this->formIDArray[i] == refID) {
      return i;
    }
  }
  return -1;
}

void SaveFile_::SaveFile::OverwritePluginInfo(
  std::vector<std::string>& newPlaginNames)
{
  uint32_t oldSize = this->pluginInfoSize;

  this->pluginInfoSize = 1;
  this->pluginInfo.numPlugins = 0;
  this->pluginInfo.pluginsName.clear();

  this->pluginInfo.numPlugins = uint8_t(newPlaginNames.size());

  for (auto& plugin : newPlaginNames) {
    this->pluginInfo.pluginsName.push_back(plugin);
    this->pluginInfoSize += uint32_t(2 + plugin.size());
  }

  uint32_t addSize = this->pluginInfoSize - oldSize;

  this->fileLocationTable.formIDArrayCountOffset += addSize;
  this->fileLocationTable.unknownTable3Offset += addSize;
  this->fileLocationTable.globalDataTable1Offset += addSize;
  this->fileLocationTable.globalDataTable2Offset += addSize;
  this->fileLocationTable.changeFormsOffset += addSize;
  this->fileLocationTable.globalDataTable3Offset += addSize;
}
