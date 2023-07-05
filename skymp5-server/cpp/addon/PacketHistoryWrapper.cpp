#include "PacketHistoryWrapper.h"

namespace {
Napi::Value ToNapiValue(const PacketHistory& history,
                        const PacketHistoryElement& element, Napi::Env env)
{
  if (element.offset >= history.buffer.size()) {
    throw std::runtime_error(
      "PacketHistoryWrapper: element.offset >= history.buffer.size()");
  }
  if (element.offset + element.length > history.buffer.size()) {
    throw std::runtime_error("PacketHistoryWrapper: element.offset + "
                             "element.length > history.buffer.size()");
  }

  const uint8_t* elementBegin = history.buffer.data() + element.offset;
  auto result = Napi::Uint8Array::New(env, element.length);

  memcpy(result.Data(), elementBegin, element.length);

  return result;
}
}

PacketHistory PacketHistoryWrapper::FromNapiValue(
  const Napi::Object& packetHistory)
{
  PacketHistory history;

  auto packets = NapiHelper::ExtractArray(packetHistory.Get("packets"),
                                          "packetHistory.packets");
  for (uint32_t i = 0; i < packets.Length(); ++i) {
    std::string tip = "packetHistory.packets." + std::to_string(i);
    std::string tip1 = "packetHistory.packets." + std::to_string(i) + ".data";
    std::string tip2 =
      "packetHistory.packets." + std::to_string(i) + ".timeMs";

    auto packet = NapiHelper::ExtractObject(packets.Get(i), tip.data());

    auto buffer =
      NapiHelper::ExtractUInt8Array(packet.Get("data"), tip1.data());
    const uint8_t* begin = buffer.Data();
    const uint8_t* end = begin + buffer.ByteLength();
    const auto previousSize = history.buffer.size();
    history.buffer.insert(history.buffer.end(), begin, end);

    PacketHistoryElement element;
    element.offset = previousSize;
    element.length = buffer.ByteLength();
    element.timeMs =
      NapiHelper::ExtractUInt32(packet.Get("timeMs"), tip2.data());

    history.packets.push_back(element);
  }

  return history;
}

Napi::Object PacketHistoryWrapper::ToNapiValue(const PacketHistory& history,
                                               Napi::Env env)
{
  auto result = Napi::Object::New(env);

  auto arr = Napi::Array::New(env, history.packets.size());
  for (int i = 0; i < static_cast<int>(history.packets.size()); ++i) {
    auto element = Napi::Object::New(env);
    element.Set("data", ::ToNapiValue(history, history.packets[i], env));
    element.Set("timeMs", Napi::Number::New(env, history.packets[i].timeMs));
    arr.Set(i, element);
  }

  result.Set("packets", arr);

  return result;
}
