#include "MovementDataSerialization.h"

#include <slikenet/BitStream.h>

namespace {
void Write(const std::array<float, 3>& arr, SLNet::BitStream& stream){
  for (size_t i = 0; i < 3; ++i) {
    stream.Write(arr[i]);
  }
}

void ReadTo(std::array<float, 3>& arr, SLNet::BitStream& stream) {
  for (size_t i = 0; i < 3; ++i) {
    stream.Read(arr[i]);
  }
}

template <class T>
T Read(SLNet::BitStream& stream) {
  T value;
  stream.Read(value);
  return value;
}
// optional
}

void Write(const MovementData& movData, SLNet::BitStream& stream) {
  stream.Write(movData.idx);
  stream.Write(movData.worldOrCell);
  Write(movData.pos, stream);
  Write(movData.rot, stream);
  stream.Write(movData.direction);
  stream.Write(movData.healthPercentage);

  stream.Write(static_cast<bool>(static_cast<uint8_t>(movData.runMode) & 2));
  stream.Write(static_cast<bool>(static_cast<uint8_t>(movData.runMode) & 1));

  stream.Write(movData.isInJumpState);
  stream.Write(movData.isSneaking);
  stream.Write(movData.isBlocking);
  stream.Write(movData.isWeapDrawn);
  if (movData.lookAt) {
    stream.Write(true);
    Write(*movData.lookAt, stream);
  } else {
    stream.Write(false);
  }
}

void ReadTo(MovementData& movData, SLNet::BitStream& stream) {
  stream.Read(movData.idx);
  stream.Read(movData.worldOrCell);
  ReadTo(movData.pos, stream);
  ReadTo(movData.rot, stream);
  stream.Read(movData.direction);
  stream.Read(movData.healthPercentage);

  uint8_t runMode = 0;
  runMode |= Read<bool>(stream);
  runMode <<= 1;
  runMode |= Read<bool>(stream);
  movData.runMode = static_cast<RunMode>(runMode);

  stream.Read(movData.isInJumpState);
  stream.Read(movData.isSneaking);
  stream.Read(movData.isBlocking);
  stream.Read(movData.isWeapDrawn);
  if (Read<bool>(stream)) {
    ReadTo(movData.lookAt.emplace(), stream);
  }
}

/*
void Serialize(const MovementData& movData, SLNet::BitStream& stream, bool isWrite) {
  stream.Serialize(movData.idx, isWrite);
  stream.Serialize(movData.worldOrCell, isWrite);
  Serialize(movData.pos, stream, isWrite);
  Serialize(movData.rot, stream, isWrite);
  stream.SerializeFloat16(isWrite, )
  if (movData.lookAt) {
    Serialize(*movData.lookAt, stream, isWrite);
  }
}
*/
