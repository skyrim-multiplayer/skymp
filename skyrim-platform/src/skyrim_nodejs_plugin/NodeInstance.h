#pragma once
#include <cstdint>
#include <memory>

class NodeInstance
{
public:
  NodeInstance();

  void Load();

  int Init();
  int CreateEnvironment(int argc, char** argv, void** outEnv);
  int DestroyEnvironment(void* env);
  int Tick(void* env);
  int ExecuteScript(void* env, const char* script);
  uint64_t GetError(char* buffer, uint64_t bufferSize);

private:
  int NodeMain(int argc, char** argv);

  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
