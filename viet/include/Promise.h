#pragma once
#include <functional>
#include <memory>
#include <stdexcept>
#include <vector>

namespace Viet {
struct Void
{
};

class AnyPromise
{
public:
  using ErrorCallback = std::function<void(const char*)>;

  template <class Promise>
  AnyPromise(const Promise& promise)
  {
    this->catchFn = [promise](ErrorCallback cb) { promise.Catch(cb); };
    this->rejectFn = [promise](const char* str) { promise.Reject(str); };
  }

  void Reject(const char* error);
  void Catch(const ErrorCallback& cb) noexcept;

private:
  std::function<void(ErrorCallback)> catchFn;
  std::function<void(const char*)> rejectFn;
};

template <class T>
class Promise
{
public:
  using Type = T;

  using ThenCallback = std::function<void(const T&)>;
  using ErrorCallback = std::function<void(const char*)>;

  const Promise<T>& Then(const ThenCallback& cb_) const noexcept
  {
    pImpl->thenCb = cb_;
    return *this;
  }

  const Promise<T>& Catch(const ErrorCallback& cb_) const noexcept
  {
    pImpl->errorCb = cb_;
    return *this;
  }

  void Then(Promise<T> promise) const noexcept
  {
    this->Then([=](const T& v) { promise.Resolve(v); });
    this->Catch([=](const char* err) { promise.Reject(err); });
  }

  void Resolve(const T& value) const
  {
    if (pImpl->pending) {
      pImpl->pending = false;
      pImpl->thenCb(value);
    }
  }

  void Reject(const char* error) const
  {
    if (pImpl->pending) {
      pImpl->pending = false;
      pImpl->errorCb(error);
    }
  }

  static Promise<std::vector<T>> All(const std::vector<Promise<T>>& promises)
  {
    Promise<std::vector<T>> res;

    struct Progress
    {
      std::vector<T> returnValues;
      size_t numDone = 0;
    };
    auto pr = std::make_shared<Progress>();
    pr->returnValues.resize(promises.size());

    for (size_t p = 0; p < promises.size(); ++p) {
      promises[p]
        .Then([pr, p, res](const T& val) {
          pr->returnValues[p] = val;
          pr->numDone++;
          if (pr->numDone == pr->returnValues.size()) {
            res.Resolve(pr->returnValues);
          }
        })
        .Catch([res](const char* err) { res.Reject(err); });
    }

    return res;
  }

  static Promise<T> Any(const std::vector<Promise<T>>& promises)
  {
    Promise<T> res;

    for (const auto& promise : promises) {
      promise.Then([res](const T& val) { res.Resolve(val); })
        .Catch([res](const char* err) { res.Reject(err); });
    }

    return res;
  }

  operator AnyPromise() { return AnyPromise(*this); }

private:
  struct Impl
  {
    ThenCallback thenCb = [](const auto&) {};
    ErrorCallback errorCb = [](const auto&) {
      throw std::runtime_error("Unhandled promise rejection");
    };
    bool pending = true;
  };

  std::shared_ptr<Impl> pImpl = std::make_shared<Impl>();
};
}
