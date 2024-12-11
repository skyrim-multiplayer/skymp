#pragma once
#include "Inventory.h"
#include "MessageBase.h"
#include "MsgType.h"
#include <type_traits>

struct CraftItemMessage : public MessageBase<CraftItemMessage>
{
  static constexpr auto kMsgType =
    std::integral_constant<char, static_cast<char>(MsgType::CraftItem)>{};

  struct Data
  {
    template <class Archive>
    void Serialize(Archive& archive)
    {
      archive.Serialize("workbench", workbench)
        .Serialize("craftInputObjects", craftInputObjects)
        .Serialize("resultObjectId", resultObjectId);
    }

    uint32_t workbench = 0;
    Inventory craftInputObjects;
    uint32_t resultObjectId = 0;
  };

  template <class Archive>
  void Serialize(Archive& archive)
  {
    archive.Serialize("t", kMsgType).Serialize("data", data);
  }

  Data data;
};
