#pragma once
#include "concepts/Concepts.h"
#include <optional>
#include <slikenet/BitStream.h>
#include <stdexcept>
#include <string>
#include <vector>

class BitStreamInputArchive
{
public:
  explicit BitStreamInputArchive(RakNet::BitStream& bitStream)
    : bs(bitStream)
  {
  }

  Raknet::Bitsrem& bs;
};
