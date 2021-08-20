#include "TestUtils.hpp"
#include <catch2/catch.hpp>

#include "ActionListener.h"
#include "EspmGameObject.h"
#include "MpObjectReference.h"
#include "PapyrusObjectReference.h"

namespace {

class TestReference : public MpObjectReference
{
public:
  TestReference(const LocationalData& locationalData,
                const FormCallbacks& callbacks, uint32_t baseId,
                std::string baseType,
                std::optional<NiPoint3> primitiveBoundsDiv2 = std::nullopt);

  void SendPapyrusEvent(const char* eventName,
                        const VarValue* arguments = nullptr,
                        size_t argumentsCount = 0) override;

  std::vector<std::string> events;
};

TestReference& CreateMpObjectReference(WorldState& worldState, uint32_t id);

TestReference& CreateMpObjectReference(PartOne& partOne, uint32_t id);

auto GetDummyMessageData();
}

