#include "NapiHelper.h"
#include "PartOne.h"

class PacketHistoryWrapper
{
public:
  static PacketHistory FromNapiValue(const Napi::Object& packetHistory);
  static Napi::Object ToNapiValue(const PacketHistory& history, Napi::Env env);
};
