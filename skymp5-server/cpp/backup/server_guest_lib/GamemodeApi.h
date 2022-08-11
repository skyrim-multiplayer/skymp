#pragma once
#include <map>
#include <string>

namespace GamemodeApi {
struct PropertyInfo
{
  std::string updateOwner;
  std::string updateNeighbor;
  bool isVisibleByNeighbors = false;
  bool isVisibleByOwner = false;
};

struct EventSourceInfo
{
  std::string functionBody;
};

struct State
{
  std::map<std::string, PropertyInfo> createdProperties;
  std::map<std::string, EventSourceInfo> createdEventSources;
};
}
