#include "EventsApi.h"

#include "InvalidArgumentException.h"
#include "MyUpdateTask.h"
#include "NullPointerException.h"
#include <map>
#include <set>

struct
{
  std::map<std::string, std::vector<JsValue>> callbacks;
} g;

void EventsApi::SendEvent(const char* eventName,
                          const std::vector<JsValue>& arguments)
{
  for (auto& f : g.callbacks[eventName])
    f.Call(arguments);
}

void EventsApi::Clear()
{
  g = {};
}

JsValue EventsApi::On(const JsFunctionArguments& args)
{
  auto eventName = args[1].ToString();
  auto callback = args[2];

  std::set<std::string> events = { "tick", "update" };

  if (events.count(eventName) == 0)
    throw InvalidArgumentException("eventName", eventName);

  g.callbacks[eventName].push_back(callback);

  return JsValue::Undefined();
}