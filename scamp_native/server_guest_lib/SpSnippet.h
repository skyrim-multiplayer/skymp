#pragma once

class MpActor;

class SpSnippet
{
public:
  SpSnippet(const char* cl_, const char* func_, const char* args_);
  void Send(MpActor* actor);

private:
  const char *const cl, *const func, *const args;
};