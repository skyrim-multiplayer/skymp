#pragma once
#include <cstdint>
#include <memory>

class NodeInstance
{
public:
  NodeInstance();

  void Load();

  int Init(int argc, char** argv);
  int CreateEnvironment(int argc, char** argv, void** outEnv);
  int DestroyEnvironment(void* env);
  int Tick(void* env);
  int CompileScript(void* env, const char* script, uint16_t scriptId);
  int ExecuteScript(void* env, uint16_t scriptId);
  uint64_t GetError(char* buffer, uint64_t bufferSize);

  const char* GetJavaScriptError();
  void ClearJavaScriptError();

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
