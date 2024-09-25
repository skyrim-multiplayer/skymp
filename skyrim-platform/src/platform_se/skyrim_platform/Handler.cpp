
#include "Handler.h"

Handler::Handler() = default;

Handler::Handler(const Napi::Value& handler_, std::optional<double> minSelfId_,
                 std::optional<double> maxSelfId_,
                 std::optional<Pattern> pattern_)
  : enter(Napi::Persistent(
      handler_.As<Napi::Object>().Get("enter").As<Napi::Function>()))
  , leave(Napi::Persistent(
      handler_.As<Napi::Object>().Get("leave").As<Napi::Function>()))
  , minSelfId(minSelfId_)
  , maxSelfId(maxSelfId_)
  , pattern(pattern_)
{
}

bool Handler::Matches(uint32_t selfId, const std::string& eventName)
{
  if (minSelfId.has_value() && selfId < minSelfId.value()) {
    return false;
  }
  if (maxSelfId.has_value() && selfId > maxSelfId.value()) {
    return false;
  }
  if (pattern.has_value()) {
    switch (pattern->type) {
      case PatternType::Exact:
        return eventName == pattern->str;
      case PatternType::StartsWith:
        return eventName.size() >= pattern->str.size() &&
          !memcmp(eventName.data(), pattern->str.data(), pattern->str.size());
      case PatternType::EndsWith:
        return eventName.size() >= pattern->str.size() &&
          !memcmp(eventName.data() + (eventName.size() - pattern->str.size()),
                  pattern->str.data(), pattern->str.size());
    }
  }
  return true;
}
