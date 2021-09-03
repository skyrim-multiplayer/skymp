#include "MovementDataSerialization.h"

#include <slikenet/BitStream.h>

namespace {
void Serialize(const std::array<float, 3>&){
  ;
}
// optional
}

void Serialize(const MovementData& movData, SLNet::BitStream& stream, bool isWrite) {
  stream.Serialize(movData.idx, isWrite);
  stream.Serialize(movData.worldOrCell, isWrite);
}
