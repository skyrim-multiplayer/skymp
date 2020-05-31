#pragma once
#include <functional>
#include <memory>

class HttpClient
{
public:
  HttpClient();
  void Update();

  using OnComplete = std::function<void(std::vector<uint8_t>)>;

  void Get(const char* host, const char* path, OnComplete callback);

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};