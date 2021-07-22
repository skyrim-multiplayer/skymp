#include "HttpClient.h"
#include "TaskQueue.h"
#include "ThreadPoolWrapper.h"
#include <filesystem>
#include <httplib.h>

struct HttpClient::Impl
{
  Impl()
    : pool(3)
  {
  }

  TaskQueue q;
  ThreadPoolWrapper pool;
};

HttpClient::HttpClient()
{
  pImpl.reset(new Impl);
}

void HttpClient::Update()
{
  pImpl->q.Update();
}

void HttpClient::Get(const char* host, const char* path, OnComplete callback)
{
  std::string path_ = path;
  std::shared_ptr<httplib::Client> cl(new httplib::Client(host));

  auto pImpl_ = pImpl;
  auto future = pImpl->pool.Push([cl, path_, callback, pImpl_](int) {
    httplib::Result res = cl->Get(path_.data());
    std::vector<uint8_t> resultVector = res
      ? std::vector<uint8_t>(res->body.begin(), res->body.end())
      : std::vector<uint8_t>();

    pImpl_->q.AddTask([callback, resultVector] { callback(resultVector); });
  });
}
