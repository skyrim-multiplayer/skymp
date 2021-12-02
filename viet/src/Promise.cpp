#include "Promise.h"

void Viet::AnyPromise::Reject(const char* error)
{
  rejectFn(error);
}

void Viet::AnyPromise::Catch(const ErrorCallback& cb) noexcept
{
  catchFn(cb);
}
