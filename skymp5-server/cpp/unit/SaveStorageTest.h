#include "TestUtils.hpp"

#include "AsyncSaveStorage.h"
#include "FileDatabase.h"
#include "MpChangeForms.h"
#include <filesystem>

std::shared_ptr<ISaveStorage> MakeSaveStorage();

MpChangeForm CreateChangeForm(const char* descStr);

void UpsertSync(ISaveStorage& st, std::vector<MpChangeForm> changeForms);

void WaitForNextUpsert(ISaveStorage& st, WorldState& wst);


