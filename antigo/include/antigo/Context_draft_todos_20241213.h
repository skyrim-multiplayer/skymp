#error "Don't include this, this was a working draft"

#include <exception>
#include <iostream>

namespace Antigo {

// TODO: layers of disablers
// 1. compile-time
// 2. runtime via static bool
// 3. runtime via static atomic bool (is 2 needed?)

// stages of rollout:
// 0. add contect to the most interesting frequently-used funcs (e.g. espm)
// 1. enable compile-time and runtime for indev
// 2. enable compile-time for sweetpie
// 3. enable runtime for sweetpie when errors occur
// ??

// struct Context {};

// template <int num>
struct Context/*WithLine : public Context*/ {
  Context/*WithLine*/() {
    std::cout << std::uncaught_exceptions() << " active exceptions"; // TODO save
  }
  Context/*WithLine*/(const Context& prev) {
    // possibly also check if it isn't cross-thread init
    std::cout << std::uncaught_exceptions() << " active exceptions"; // TODO save
    // std::cout << "hi from ContextWithLine line " << num << "\n";
    (void)prev;
  }

  ~Context() {
    // check if all next-s were destructed

    std::cout << std::uncaught_exceptions() << " active exceptions";
    // save this context to this thread's store
    // if this is called, we're in the scope of an exception
    // try to capture the message from some exception if it's possible
  }

  // only to be called by the macro
  void AddLineFunc(const char* file, int num, const char* func) {
    // save location data
    // also save a fully-fledged stacktrace if configured to do so
    std::cout << "hi from ContextWithLine after AddLineFunc " <<file << ":" << num << " - " << func << "\n";
  }

  template <class T>
  void AddKey(std::string_view key, T val);
  // put to std::any?

  template <class T>
  T GetKey(std::string_view key);

  // we can get these messages, e.g. in case of an exception
  // TODO: should be disableable with 0 performance loss
  // alternatively, return spdlog logger that logs to this context
  // maybe context should receive spdlog for this purpose
  template <class... T>
  void AddMessage(T&&... spdlogArgs);

  // get a message that includes:
  // 1. context trace/chain, full stacktrace if it was captured
  // 2. messages output since each context in the chain was created
  // 3. keys that were set
  // example use: catch (const weird_exception& e) { GetLastContext().FormatMessage() }
  std::string FormatMessage();
};

#define ANTIGO_CONTEXT Context //WithLine<__LINE__>
#define ANTIGO_CONTEXT_INIT(ctx) ctx.AddLineFunc(__FILE__, __LINE__, __func__)

struct ThreadData
{
  // TODO: align to avoid https://en.wikipedia.org/wiki/False_sharing

  // stack of contexts
  // 1/5/10 recent deepest contexts destroyed with an exception (i.e. excl ones that were destructed via stack unwinding that also destroyed another context)
};

// returns this thread's deepest context
const Context& GetLastContext();

// return thread locla
ThreadData& GetThreadData();

// prints topmost data from the latest context
// should also print a command to run to debug the crash with gdb
void SetupTerminateHandler();

// TODO: detect catch that didn't pop exception witness
/*
  0 << thrown
  1
  2
  3 << caught (in scope of this context)
  ------ unwinding 0-3
  4
  5
  6 << detected
  7
*/
// maybe we should detect in destructor of 3/4?
// probably in 3. We can see that we're no longer in the stack unwinding process
// then we probably push it to some other struct and let some loop fetch unhandled witnesses

}
