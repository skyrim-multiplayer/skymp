#include "HttpClient.h"
#include "TaskQueue.h"
#include "ThreadPoolWrapper.h"
#include <filesystem>

#define CPPHTTPLIB_OPENSSL_SUPPORT
#include <httplib.h>

namespace {
inline httplib::Headers CastHeaders(const HttpClient::Headers& headers)
{
  httplib::Headers res;
  for (auto& p : headers) {
    res.insert(p);
  }
  return res;
}
}

struct HttpClient::Impl
{
  Impl()
    : pool(3)
  {
  }

  Viet::TaskQueue q;
  ThreadPoolWrapper pool;
};

HttpClient::HttpClient()
{
  pImpl.reset(new Impl);
}

void HttpClient::ExecuteQueuedCallbacks()
{
  pImpl->q.Update();
}

void HttpClient::Get(const char* host, const char* path,
                     const Headers& headers, OnComplete callback)
{
  if (path[0] && path[0] != '/') {
    throw std::runtime_error("HTTP paths must start with '/'");
  }

  std::string path_ = path;
  auto cl = std::make_shared<httplib::Client>(host);

  auto pImpl_ = pImpl;
  pImpl->pool.Push([cl, path_, headers, callback, pImpl_] {
    httplib::Result res = cl->Get(path_.data(), CastHeaders(headers));
    auto resultVector = res
      ? std::vector<uint8_t>(res->body.begin(), res->body.end())
      : std::vector<uint8_t>();
    int32_t status = res ? res->status : 0;
    std::string error = res ? std::string{} : to_string(res.error());

    pImpl_->q.AddTask([callback, resultVector, status, error] {
      callback({ resultVector, status, error });
    });
  });
}

void HttpClient::Post(const char* host, const char* path, const char* body,
                      const char* contentType, const Headers& headers,
                      OnComplete callback)
{
  if (path[0] && path[0] != '/') {
    throw std::runtime_error("HTTP paths must start with '/'");
  }

  std::string path_ = path;
  std::string body_ = body;
  std::string contentType_ = contentType;
  auto cl = std::make_shared<httplib::Client>(host);

  auto pImpl_ = pImpl;
  pImpl->pool.Push(
    [cl, path_, body_, contentType_, headers, callback, pImpl_] {
      httplib::Result res =
        cl->Post(path_.data(), CastHeaders(headers), body_.data(),
                 body_.size(), contentType_.data());
      auto resultVector = res
        ? std::vector<uint8_t>(res->body.begin(), res->body.end())
        : std::vector<uint8_t>();
      int32_t status = res ? res->status : 0;

      pImpl_->q.AddTask([callback, resultVector, status] {
        callback({ resultVector, status });
      });
    });
}
