#include "MessageSerializerFactory.h"
#include "MpClientPlugin.h"
#include <cstdint>
#include <nlohmann/json.hpp>

namespace {
MpClientPlugin::State& GetState()
{
  static MpClientPlugin::State g_state;
  return g_state;
}

MessageSerializer& GetMessageSerializer()
{
  static std::shared_ptr<MessageSerializer> g_serializer =
    MessageSerializerFactory::CreateMessageSerializer();
  return *g_serializer;
}

void MySerializeMessage(const char* jsonContent,
                        SLNet::BitStream& outputStream)
{
  GetMessageSerializer().Serialize(jsonContent, outputStream);
}

bool MyDeserializeMessage(const uint8_t* data, size_t length,
                          std::string& outJsonContent)
{
  std::optional<DeserializeResult> result =
    GetMessageSerializer().Deserialize(data, length);
  if (!result) {
    return false;
  }

  // TODO(perf): there should be a faster way to get JS object from binary
  // (without extra json building)
  nlohmann::json outJson;
  result->message->WriteJson(outJson);
  outJsonContent = outJson.dump();
  return true;
}
}

extern "C" {
__declspec(dllexport) const char* MpCommonGetVersion()
{
  return "0.0.1";
}

__declspec(dllexport) void CreateClient(const char* targetHostname,
                                        uint16_t targetPort)
{
  return MpClientPlugin::CreateClient(GetState(), targetHostname, targetPort);
}

__declspec(dllexport) void DestroyClient()
{
  return MpClientPlugin::DestroyClient(GetState());
}

__declspec(dllexport) bool IsConnected()
{
  return MpClientPlugin::IsConnected(GetState());
}

__declspec(dllexport) void Tick(MpClientPlugin::OnPacket onPacket, void* state)
{

  return MpClientPlugin::Tick(GetState(), onPacket, MyDeserializeMessage,
                              state);
}

__declspec(dllexport) void Send(const char* jsonContent, bool reliable)
{
  return MpClientPlugin::Send(GetState(), jsonContent, reliable,
                              MySerializeMessage);
}
}
