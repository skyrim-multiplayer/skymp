#pragma once
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include <napi.h>

class HttpClient
{
public:
  HttpClient();
  void ExecuteQueuedCallbacks(Napi::Env env);

  struct HttpResult
  {
    std::vector<uint8_t> body;
    int32_t status = 0;
    std::string error;
  };

  using Headers = std::vector<std::pair<std::string, std::string>>;

  using OnComplete = std::function<void(Napi::Env, HttpResult)>;

  void Get(const char* host, const char* path, const Headers& headers,
           OnComplete callback);
  void Post(const char* host, const char* path, const char* body,
            const char* contentType, const Headers& headers,
            OnComplete callback);

private:
  struct Impl;
  std::shared_ptr<Impl> pImpl;
};
