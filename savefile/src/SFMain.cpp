#include "savefile/SFChangeFormACHR.h"
#include "savefile/SFChangeFormNPC.h"
#include "savefile/SFReader.h"
#include "savefile/SFSeekerOfDifferences.h"
#include "savefile/SFStructure.h"
#include "savefile/SFWriter.h"

#include <filesystem>

namespace fs = std::filesystem;

using namespace SaveFile_;

void ChangeFormNPC(std::shared_ptr<SaveFile> structure,
                   ChangeFormNPC_& newForm) noexcept
{
  std::vector<ChangeForm>& changeForms = structure->changeForms;
  for (auto& formObj : changeForms) {

    if (!formObj.Is_NPC_Type())
      continue;

    if (!formObj.formID.IsPlayerBaseID())
      continue;

    const auto& form = newForm.ToBinary();

    structure->fileLocationTable.formIDArrayCountOffset -= formObj.length1;
    structure->fileLocationTable.formIDArrayCountOffset += form.second.size();

    structure->fileLocationTable.unknownTable3Offset -= formObj.length1;
    structure->fileLocationTable.unknownTable3Offset += form.second.size();

    structure->fileLocationTable.globalDataTable3Offset -= formObj.length1;
    structure->fileLocationTable.globalDataTable3Offset += form.second.size();

    formObj.length2 = 0;
    formObj.length1 = form.second.size();
    formObj.data = form.second;
    formObj.changeFlags = form.first;
  }
}

void ChangeFormACHR(std::shared_ptr<SaveFile> structure)
{
  ChangeFormACHR_ form;
  auto res = form.ToBinary();

  std::vector<ChangeForm>& changeForms = structure->changeForms;
  for (auto& formObj : changeForms) {

    if (!formObj.Is_ACHR_Type())
      continue;

    if (!formObj.formID.IsPlayerID())
      continue;

    structure->fileLocationTable.formIDArrayCountOffset -= formObj.length1;
    structure->fileLocationTable.formIDArrayCountOffset += res.second.size();

    structure->fileLocationTable.unknownTable3Offset -= formObj.length1;
    structure->fileLocationTable.unknownTable3Offset += res.second.size();

    structure->fileLocationTable.globalDataTable3Offset -= formObj.length1;
    structure->fileLocationTable.globalDataTable3Offset += res.second.size();

    formObj.length2 = 0;
    formObj.length1 = res.second.size();
    formObj.data = res.second;
    formObj.changeFlags = res.first;
  }
}

int savefile_main(int argc, char* argv[])
{
  setlocale(LC_ALL, "Russian");

  std::vector<std::string> allPath;
  std::vector<fs::path> essFiles;

  const fs::path pathTo = fs::current_path();
  fs::directory_iterator begin("ess");
  fs::directory_iterator end;

  std::copy_if(
    begin, end, std::back_inserter(essFiles), [](const fs::path& path) {
      return fs::is_regular_file(path) && (path.extension() == ".ess");
    });

  for (auto file : essFiles) {
    allPath.push_back(fs::absolute(file).generic_string());
  }

  Reader reader1(allPath.at(0));
  Reader reader2(allPath.at(1));

  SeekerOfDifferences diff;

  /*std::vector<RefID> headParts = { RefID(0x0005161C) ,  RefID(0x00053997) ,
  RefID(0x0008CA6F) ,  RefID(0x000E4D9F) }; std::vector<float> options;
  std::vector<uint32_t> presets = {0, 0, 0, 0};

  options.resize(19);

  ChangeFormNPC_ npcForm;
  npcForm.playerName = "Darkwood";
  npcForm.gender = 1;
  npcForm.race = { RefID(0x00013745) , RefID(0x00013745) };

  npcForm.face = { RefID(0x000E6E0B, {0}, RefID(0x0006A1B0) ,headParts,
  options, presets };


  ChangeFormNPC(reader1.GetStructure(), npcForm);*/

  // ChangeFormACHR(reader1.GetStructure());

  diff.AddToComparisonFirst(reader1.GetStructure());
  diff.AddToComparisonSecond(reader2.GetStructure());

  auto result = diff.CompareAddedObjects();

  std::cout << "Number of differences is " << result.size() << std::endl;

  /*diff.writer.open("NPC_form_1_0x7TintMask.txt", std::ios::binary);
  diff.CoutVector(result.at(0)[0].value, essFiles.at(0).generic_string() + "
  NPC form "); diff.writer.close();

  diff.writer.open("NPC_form_2_0x7TintMask.txt", std::ios::binary);
  diff.CoutVector(result.at(0)[1].value, essFiles.at(1).generic_string() + "
  NPC form "); diff.writer.close();*/

  Writer writer(reader1.GetStructure());

  writer.CreateSaveFile("SkyMPSave.ess");

  std::string pathForNewSave = pathTo.generic_string() + "/SkyMPSave.ess";

  return 0;
}
