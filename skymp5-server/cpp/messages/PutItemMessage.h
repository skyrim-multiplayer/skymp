#include "Inventory.h"
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct PutItemMessage
  : public MessageBase<PutItemMessage>
  , public Inventory::ExtraData
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::PutItem)>{};

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType)
      .Serialize("baseId", baseId)
      .Serialize("count", count)
      .Serialize("target", target);

    ExtraData::Serialize(archive);
  }

  uint32_t baseId = 0;
  uint32_t count = 0;
  uint32_t target = 0;
};
