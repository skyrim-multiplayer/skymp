#pragma once
#include <functional>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include "Void.h"

namespace Viet {

// ===== Storage Policies =====

struct SharedStorage
{
  template <class Impl>
  struct Holder
  {
    std::shared_ptr<Impl> ptr = std::make_shared<Impl>();

    Impl* get() const noexcept { return ptr.get(); }
  };
};

struct InlineStorage
{
  template <class Impl>
  struct Holder
  {
    Impl data{};

    Impl* get() noexcept { return &data; }
    const Impl* get() const noexcept { return &data; }

    Holder() = default;
    Holder(const Holder&) = delete;
    Holder& operator=(const Holder&) = delete;
    Holder(Holder&&) = default;
    Holder& operator=(Holder&&) = default;
  };
};

// ===== Forward declarations =====

template <class T, class ThenCb, class ErrorCb, class Storage>
class BasicPromise;

template <class T>
using Promise = BasicPromise<T, std::function<void(const T&)>,
                             std::function<void(const char*)>, SharedStorage>;

// ===== AnyPromise =====

class AnyPromise
{
public:
  using ErrorCallback = std::function<void(const char*)>;

  template <class PromiseType>
  AnyPromise(PromiseType promise)
  {
    this->catchFn = [promise](ErrorCallback cb) mutable { promise.Catch(cb); };
    this->rejectFn = [promise](const char* str) mutable {
      promise.Reject(str);
    };
  }

  void Reject(const char* error);
  void Catch(const ErrorCallback& cb) noexcept;

private:
  std::function<void(ErrorCallback)> catchFn;
  std::function<void(const char*)> rejectFn;
};

// ===== BasicPromise =====

template <class T, class ThenCb = std::function<void(const T&)>,
          class ErrorCb = std::function<void(const char*)>,
          class Storage = SharedStorage>
class BasicPromise
{
public:
  using Type = T;
  using ThenCallback = ThenCb;
  using ErrorCallback = ErrorCb;
  using StoragePolicy = Storage;

  BasicPromise& Then(ThenCallback cb) noexcept
  {
    impl()->thenCb = std::move(cb);
    return *this;
  }

  BasicPromise& Catch(ErrorCallback cb) noexcept
  {
    impl()->errorCb = std::move(cb);
    return *this;
  }

  template <class OtherPromise>
  void Forward(OtherPromise promise) noexcept
  {
    this->Then([promise](const T& v) mutable { promise.Resolve(v); });
    this->Catch([promise](const char* err) mutable { promise.Reject(err); });
  }

  void Resolve(const T& value)
  {
    if (impl()->pending) {
      impl()->pending = false;
      if (impl()->thenCb) {
        impl()->thenCb(value);
      }
    }
  }

  void Reject(const char* error)
  {
    if (impl()->pending) {
      impl()->pending = false;
      if (impl()->errorCb) {
        impl()->errorCb(error);
      } else {
        throw std::runtime_error("Unhandled promise rejection");
      }
    }
  }

  // All and Race are only available for SharedStorage (copyable promises)
  template <class S = Storage>
  static auto All(const std::vector<Promise<T>>& promises)
    -> std::enable_if_t<std::is_same_v<S, SharedStorage>,
                        Promise<std::vector<T>>>
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
      // Copy promises[p] to allow mutation in lambdas
      auto promiseCopy = promises[p];
      promiseCopy
        .Then([pr, p, res](const T& val) mutable {
          pr->returnValues[p] = val;
          pr->numDone++;
          if (pr->numDone == pr->returnValues.size()) {
            res.Resolve(pr->returnValues);
          }
        })
        .Catch([res](const char* err) mutable { res.Reject(err); });
    }

    return res;
  }

  template <class S = Storage>
  static auto Race(const std::vector<Promise<T>>& promises)
    -> std::enable_if_t<std::is_same_v<S, SharedStorage>, Promise<T>>
  {
    Promise<T> res;

    for (const auto& promise : promises) {
      auto promiseCopy = promise;
      promiseCopy.Then([res](const T& val) mutable { res.Resolve(val); })
        .Catch([res](const char* err) mutable { res.Reject(err); });
    }

    return res;
  }

  operator AnyPromise() { return AnyPromise(*this); }

private:
  struct Impl
  {
    ThenCallback thenCb{};
    ErrorCallback errorCb{};
    bool pending = true;
  };

  typename Storage::template Holder<Impl> storage_;

  Impl* impl() noexcept { return storage_.get(); }
  const Impl* impl() const noexcept { return storage_.get(); }
};

// ===== Aliases =====

template <class T, class ThenCb = std::function<void(const T&)>,
          class ErrorCb = std::function<void(const char*)>>
using MoveOnlyPromise = BasicPromise<T, ThenCb, ErrorCb, InlineStorage>;

}
