#include <cstddef>
#include <cstdint>
#include <exception>
#include <random>
#include <stdexcept>
#include <stop_token>
#include <string>
#include <string_view>
#include <vector>

#include <catch2/catch_all.hpp>

#include "antigo/Context.h"
#include "antigo/ResolvedContext.h"
#include "antigo/ExecutionData.h"
#include "antigo/impl/test_helpers.h"

namespace {

// expects messages in order of calls
struct MessageWalker
{
  const std::vector<Antigo::ResolvedMessageEntry>& v;

  size_t i = 0;
  bool skipped = false;

  [[nodiscard]]
  bool HasCstr(std::string_view msg) {
    for (; i < v.size(); ++i) {
      if (v[i].type == "cstr" && v[i].strval == msg) {
        ++i;
        return true;
      }
      skipped = true;
    }
    return false;
  }

  [[nodiscard]]
  bool Has(const std::string& type, const std::string& message) {
    for (; i < v.size(); ++i) {
      // XXX: fix tests
      if (v[i].type == type && (type == "uint" || type == "int" || v[i].strval == message)) {
        ++i;
        return true;
      }
      skipped = true;
    }
    return false;
  }

  [[nodiscard]]
  bool HasStacktrace() {
    for (; i < v.size(); ++i) {
      if (v[i].type == "stacktrace") {
        ++i;
        return true;
      }
      skipped = true;
    }
    return false;
  }

  bool MatchedAll() const {
    return i == v.size() && !skipped;
  }
};

struct CustomTestException : public std::exception {
  virtual const char* what() const noexcept override {
    return "custom test exception";
  }
};

}

TEST_CASE("Test context simple")
{
  struct {
    size_t lineThrowerFunc;
    void ThrowerFunc() {
      ANTIGO_CONTEXT_INIT(ctx);
      lineThrowerFunc = __LINE__ - 1;
      ctx.AddMessage("gonna throw an error now");
      throw std::runtime_error("boo!");
    };
  } f;

  REQUIRE_THROWS(f.ThrowerFunc());
  REQUIRE_FALSE(Antigo::HasExceptionWitness());
  REQUIRE(Antigo::HasExceptionWitnessOrphan());
  auto w = Antigo::PopExceptionWitnessOrphan();
  REQUIRE(w.entries.size() == 1);
  REQUIRE(w.entries[0].sourceLoc.filename);
  REQUIRE(std::string{w.entries[0].sourceLoc.filename} == __FILE__);
  REQUIRE(w.entries[0].sourceLoc.line == f.lineThrowerFunc);
  REQUIRE(w.entries[0].sourceLoc.func);
  REQUIRE(std::string{w.entries[0].sourceLoc.func} == "ThrowerFunc");
  MessageWalker wlk{w.entries[0].messages};
  REQUIRE(wlk.HasCstr("gonna throw an error now"));

  REQUIRE(Antigo::impl::HasCleanState());
}

TEST_CASE("Test context multifunc exception (with picked up witnesses)")
{
  struct {
    std::vector<Antigo::ResolvedContext> collected;

    size_t lineThrowerFunc;
    void ThrowerFunc(bool doThrow) {
      ANTIGO_CONTEXT_INIT(ctx);
      lineThrowerFunc = __LINE__ - 1;
      if (doThrow) {
        throw std::runtime_error("error from Foo");
      }
    };

    size_t lineCatcherFunc;
    void CatcherFunc() {
      ANTIGO_CONTEXT_INIT(ctx);
      lineCatcherFunc = __LINE__ - 1;
      ctx.AddMessage("before throw 1");
      try {
        ThrowerFunc(true);
        ctx.AddMessage("you won't see this message");
      } catch (const std::exception& e) {
        if (Antigo::HasExceptionWitness()) {
          auto w = Antigo::PopExceptionWitness();
          ctx.AddMessage("caught 1, but this w won't have it");
          collected.push_back(std::move(w));
        } else {
          throw std::runtime_error("caught 1, but no witness");
        }
        if (Antigo::HasExceptionWitness()) {
          throw std::runtime_error("caught 1, only expected one witness");
        }
      }
      ctx.AddMessage("before throw 2");
      try {
        ThrowerFunc(true);
      } catch (const std::exception& e) {
        if (Antigo::HasExceptionWitness()) {
          collected.push_back(Antigo::PopExceptionWitness());
        } else {
          throw std::runtime_error("caught 2, only expected one witness");
        }
      }
      ctx.AddMessage("after throw 2 (you won't see it)");
      ThrowerFunc(false); // won't throw
    }
  } f;

  f.CatcherFunc();
  REQUIRE_FALSE(Antigo::HasExceptionWitness()); // should be collected
  REQUIRE(f.collected.size() == 2);

  {
    auto& ent0 = f.collected[0].entries;
    REQUIRE(ent0.size() == 2);

    REQUIRE(ent0[0].sourceLoc.line == f.lineThrowerFunc);
    REQUIRE(ent0[0].messages.size() == 1);
    REQUIRE(ent0[0].messages[0].type == "stacktrace");

    REQUIRE(ent0[1].sourceLoc.line == f.lineCatcherFunc);
    MessageWalker wlk{ent0[1].messages};
    REQUIRE(wlk.HasCstr("before throw 1"));
    REQUIRE(wlk.MatchedAll());
  }

  {
    auto& ent1 = f.collected[1].entries;
    REQUIRE(ent1.size() == 2);

    REQUIRE(ent1[0].sourceLoc.line == f.lineThrowerFunc);
    REQUIRE(ent1[0].messages.size() == 1);
    REQUIRE(ent1[0].messages[0].type == "stacktrace");

    REQUIRE(ent1[1].sourceLoc.line == f.lineCatcherFunc);
    MessageWalker wlk{ent1[1].messages};
    REQUIRE(wlk.HasCstr("before throw 1"));
    REQUIRE(wlk.HasCstr("caught 1, but this w won't have it"));
    REQUIRE(wlk.HasCstr("before throw 2"));
    REQUIRE(wlk.MatchedAll());
  }

  REQUIRE(Antigo::impl::HasCleanState());
}

TEST_CASE("Test detached context")
{
  struct {
    std::vector<Antigo::ResolvedContext> collected;

    size_t lineThrowerFunc;
    void ThrowerFunc() {
      ANTIGO_CONTEXT_INIT(ctx);
      lineThrowerFunc = __LINE__ - 1;
      throw std::runtime_error("error from Foo");
    };

    size_t lineCatcherFunc;
    void CatcherFunc() {
      ANTIGO_CONTEXT_INIT(ctx);
      lineCatcherFunc = __LINE__ - 1;
      try {
        ThrowerFunc();
      } catch (const std::exception& e) {
        // doesn't pop witness
      }
    }
  } f;

  f.CatcherFunc();

  // XXX do it like this??
  REQUIRE(!Antigo::HasExceptionWitness());
  REQUIRE(Antigo::HasExceptionWitnessOrphan());
  auto w = Antigo::PopExceptionWitnessOrphan();
  REQUIRE(w.entries.size() == 2);

  REQUIRE(Antigo::impl::HasCleanState());
}

TEST_CASE("Test recursive")
{
  struct HelperF {
    bool popWitness = false;
    int catchDepth = -1;
    int throwDepth = -1;
    int exitDepth = -1;

    struct DepthData {
      bool hasWitness{};
      bool hasWitnessOrphan{};
      bool thrown{};
      bool caught{};
      bool exited{};
    };
    std::vector<DepthData> dd;

    int orphanAppearedDepth = -1;
    int caughtDepth = -1;
    int thrownDepth = -1;
    int exitedDepth = -1;

    void UpdateWitnessStats(DepthData& data, size_t currentDepth) {
      data.hasWitness = Antigo::HasExceptionWitness();
      data.hasWitnessOrphan = Antigo::HasExceptionWitnessOrphan();
      if (data.hasWitnessOrphan && orphanAppearedDepth == -1) {
        orphanAppearedDepth = currentDepth;
      }
    }

    size_t lineRecursiveHelper;
    void RecursiveHelper(size_t currentDepth) {
      ANTIGO_CONTEXT_INIT(ctx);
      lineRecursiveHelper = __LINE__ - 1;

      assert(dd.size() == currentDepth);
      dd.emplace_back();

      // exit conditions
      if (currentDepth == exitDepth) {
        exitedDepth = currentDepth;
        dd[currentDepth].exited = true;
        return;
      }
      if (currentDepth == throwDepth) {
        thrownDepth = currentDepth;
        dd[currentDepth].thrown = true;
        throw CustomTestException();
      }

      // do next call
      if (currentDepth == catchDepth) {
        try {
          RecursiveHelper(currentDepth + 1);
          UpdateWitnessStats(dd[currentDepth], currentDepth);
        } catch (const CustomTestException& e) {
          caughtDepth = currentDepth;
          UpdateWitnessStats(dd[currentDepth], currentDepth);
          if (popWitness && Antigo::HasExceptionWitness()) {
            Antigo::PopExceptionWitness();
          }
        }
        return;
      }
      RecursiveHelper(currentDepth + 1);
      UpdateWitnessStats(dd[currentDepth], currentDepth);
    }
  };

  {
    HelperF f;
    f.exitDepth = 5;
    f.RecursiveHelper(0);
    REQUIRE(f.dd.size() == 6);
    REQUIRE(f.orphanAppearedDepth == -1);
    REQUIRE(f.thrownDepth == -1);
    REQUIRE(f.exitedDepth == 5);
    REQUIRE(Antigo::impl::HasCleanState());
  }

  {
    HelperF f;
    f.exitDepth = 5;
    f.RecursiveHelper(0);
    REQUIRE(f.dd.size() == 6);
    REQUIRE(f.orphanAppearedDepth == -1);
    REQUIRE(f.thrownDepth == -1);
    REQUIRE(f.exitedDepth == 5);
    REQUIRE(Antigo::impl::HasCleanState());
  }

  {
    HelperF f;
    f.throwDepth = 0;
    f.exitDepth = 5;
    REQUIRE_THROWS_AS(f.RecursiveHelper(0), CustomTestException);
    REQUIRE(f.dd.size() == 1);
    REQUIRE(f.orphanAppearedDepth == -1);
    REQUIRE(f.thrownDepth == 0);
    REQUIRE(f.exitedDepth == -1);
    REQUIRE_FALSE(Antigo::HasExceptionWitness());
    REQUIRE(Antigo::HasExceptionWitnessOrphan());
    Antigo::PopExceptionWitnessOrphan();
    REQUIRE_FALSE(Antigo::HasExceptionWitnessOrphan());
    REQUIRE(Antigo::impl::HasCleanState());
  }

  {
    HelperF f;
    f.throwDepth = 1;
    f.catchDepth = 0;
    f.exitDepth = 5;
    f.RecursiveHelper(0);
    REQUIRE(f.dd.size() == 2);
    REQUIRE(f.orphanAppearedDepth == -1); // after returning from RecursiveHelper
    REQUIRE(f.thrownDepth == 1);
    REQUIRE(f.caughtDepth == 0);
    REQUIRE(f.exitedDepth == -1);
    REQUIRE_FALSE(Antigo::HasExceptionWitness());
    REQUIRE(Antigo::HasExceptionWitnessOrphan());
    Antigo::PopExceptionWitnessOrphan();
    REQUIRE_FALSE(Antigo::HasExceptionWitnessOrphan());
    REQUIRE(Antigo::impl::HasCleanState());
  }

  {
    HelperF f;
    f.throwDepth = 2;
    f.catchDepth = 1;
    f.exitDepth = 5;
    f.RecursiveHelper(0);
    REQUIRE(f.dd.size() == 3);
    REQUIRE(f.orphanAppearedDepth == 0);
    REQUIRE(f.thrownDepth == 2);
    REQUIRE(f.caughtDepth == 1);
    REQUIRE(f.exitedDepth == -1);
    REQUIRE_FALSE(Antigo::HasExceptionWitness());
    REQUIRE(Antigo::HasExceptionWitnessOrphan());
    Antigo::PopExceptionWitnessOrphan();
    REQUIRE_FALSE(Antigo::HasExceptionWitnessOrphan());
    REQUIRE(Antigo::impl::HasCleanState());
  }

  REQUIRE(Antigo::impl::HasCleanState());
}

TEST_CASE("Test several catches with no pop")
{
  struct HelperF {
    std::vector<Antigo::ResolvedContext> popped;

    void ThrowerFunc() {
      ANTIGO_CONTEXT_INIT(ctx);
      throw CustomTestException();
    }

    void CatcherFunc(bool doPop) {
      ANTIGO_CONTEXT_INIT(ctx);
      try {
        ThrowerFunc();
      } catch (const CustomTestException&) {
        // ignore
      }
      try {
        ThrowerFunc();
      } catch (const CustomTestException&) {
        // ignore
      }
      try {
        ThrowerFunc();
      } catch (const CustomTestException&) {
        // ignore
      }

      if (Antigo::HasExceptionWitnessOrphan()) {
        throw std::runtime_error("orphan unexpected");
      }

      if (doPop) {
        while (Antigo::HasExceptionWitness()) {
          popped.push_back(Antigo::PopExceptionWitness());
        }
      }
    }
  };

  {
    HelperF f;
    f.CatcherFunc(false);
    REQUIRE(f.popped.size() == 0);
    size_t witnessCnt = 0;
    while (Antigo::HasExceptionWitness()) {
      Antigo::PopExceptionWitness();
      ++witnessCnt;
    }
    REQUIRE(witnessCnt == 0);
    size_t orphansCnt = 0;
    while (Antigo::HasExceptionWitnessOrphan()) {
      Antigo::PopExceptionWitnessOrphan();
      ++orphansCnt;
    }
    REQUIRE(orphansCnt == 3);
    // TODO: check via lines or messages?
    REQUIRE(Antigo::impl::HasCleanState());
  }

  {
    HelperF f;
    f.CatcherFunc(true);
    REQUIRE(f.popped.size() == 3);
    // TODO: check via lines or messages?
    REQUIRE(Antigo::impl::HasCleanState());
  }

  REQUIRE(Antigo::impl::HasCleanState());
}

namespace {
struct alignas(64) Kek {
  std::mt19937 gen;
  std::bernoulli_distribution rngUnlikely;
  std::bernoulli_distribution rngCoin;
  std::bernoulli_distribution rngLikely;

  std::uint32_t iterations = 0;

  Kek(size_t seed): gen(seed), rngUnlikely(0.2), rngCoin(0.5), rngLikely(0.9), iterations(0) {}

  void RecursiveFunc(std::stop_token stop_token, size_t depth) {
    ANTIGO_CONTEXT_INIT(ctx);
    if (stop_token.stop_requested()) {
      return;
    }
    if (rngUnlikely(gen)) {
      return;
    }
    iterations++;
    if (rngUnlikely(gen)) {
      throw CustomTestException();
    }
    if (depth < 50 && rngLikely(gen)) {
      if (rngUnlikely(gen)) {
        try {
          RecursiveFunc(stop_token, depth + 1);
        } catch (const CustomTestException&) {
          // ignore
        }
      } else {
        RecursiveFunc(stop_token, depth + 1);
      }

      if (rngUnlikely(gen)) {
        while (Antigo::HasExceptionWitness()) {
          Antigo::PopExceptionWitness();
        }
      }

      if (rngUnlikely(gen)) {
        while (Antigo::HasExceptionWitnessOrphan()) {
          Antigo::PopExceptionWitnessOrphan();
        }
      }
    }
  }

  void EntryFunc(std::stop_token stop_token) {
    ANTIGO_CONTEXT_INIT(ctx);
    while (!stop_token.stop_requested()) {
      try {
        RecursiveFunc(stop_token, 1);
      } catch (const CustomTestException&) {
        // ignore
      }
    }
  }
};
}  // namespace

// XXX 20250112 fix Ubuntu build
/*
TEST_CASE("Test recursive random")
{
  uint32_t seed = Catch::getSeed();
  std::vector<Kek> keks;
  std::vector<std::jthread> threads;
  keks.reserve(4);
  threads.reserve(4);
  for (size_t i = 0; i < 4; ++i) {
    auto& kek = keks.emplace_back(seed + i);
    threads.emplace_back(&Kek::EntryFunc, kek);
    // XXX ^ fails on clang 15 and MSVC
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(5000));

  REQUIRE(threads.size() == keks.size());
  threads.clear(); // jthreads join

  std::vector<size_t> iterations;
  for (size_t i = 0; i < keks.size(); ++i) {
    iterations.push_back(keks[i].iterations);
  }
  CAPTURE(iterations);

  REQUIRE(Antigo::impl::HasCleanState());
}
*/

TEST_CASE("Test context messages")
{
  struct {
    size_t evaluated1 = false;
    size_t evaluated2 = false;
    size_t evaluated3 = false;
    size_t evaluated4 = false;
    size_t evaluated5 = false;

    void InnerFunc() {
      ANTIGO_CONTEXT_INIT(ctx);

      ctx.AddMessage("logging some predefined string");
      ctx.AddUnsigned(1337);
      ctx.AddSigned(-1337);

      std::vector<int> v{1, 2, 3};
      ctx.AddLambdaWithOwned([this, v = std::move(v)]() {
        evaluated1++;
        return "captured a vector that has " + std::to_string(v.size()) + " elements";
      });

      std::vector<int> v2{1, 2, 3, 4};
      auto g = ctx.AddLambdaWithRef([this, &v2]() {
        evaluated2++;
        return "vector has " + std::to_string(v2.size()) + " elements; this one won't expire after Orphan but will expire after exception";
      });
      g.Arm();

      {
        ctx.AddLambdaWithOwned([this]() {
          evaluated3++;
          return "owned, will be copied";
        });

        auto g2 = ctx.AddLambdaWithRef([this]() {
          evaluated4++;
          return "ref, will expire";
        });
        g2.Arm();
      }

      ctx.Orphan();
      throw CustomTestException();
    }

    void OuterFunc() {
      ANTIGO_CONTEXT_INIT(ctx);

      ctx.AddLambdaWithOwned([this]() {
        evaluated5++;
        return "some message from the outer scope just to verify proper capture of the whole chain";
      });

      InnerFunc();
    }
  } f;

  REQUIRE_THROWS_AS(f.OuterFunc(), CustomTestException);

  const std::string expiredMessage = "(reference possibly expired or Arm() not called)";

  REQUIRE(Antigo::HasExceptionWitnessOrphan());
  {
    auto w = Antigo::PopExceptionWitnessOrphan();
    REQUIRE(w.reason == "exception");
    auto& e = w.entries;
    REQUIRE(e.size() == 2);
    {
      REQUIRE(std::string_view{e[0].sourceLoc.func} == "InnerFunc");
      MessageWalker wlk{e[0].messages};
      REQUIRE(wlk.Has("cstr", "logging some predefined string"));
      REQUIRE(wlk.Has("uint", "1337"));
      REQUIRE(wlk.Has("int", "-1337"));
      REQUIRE(wlk.Has("custom", "captured a vector that has 3 elements"));
      REQUIRE(wlk.Has("custom", expiredMessage));
      REQUIRE(wlk.Has("custom", "owned, will be copied"));
      REQUIRE(wlk.Has("custom", expiredMessage));
      REQUIRE(wlk.HasStacktrace());
      REQUIRE(wlk.MatchedAll());
    }
    {
      REQUIRE(std::string_view{e[1].sourceLoc.func} == "OuterFunc");
      MessageWalker wlk{e[1].messages};
      REQUIRE(wlk.Has("custom", "some message from the outer scope just to verify proper capture of the whole chain"));
      REQUIRE(wlk.MatchedAll());
    }
  }

  REQUIRE(Antigo::HasExceptionWitnessOrphan());
  {
    auto w = Antigo::PopExceptionWitnessOrphan();
    REQUIRE(w.reason == "ondemand");
    auto& e = w.entries;
    REQUIRE(e.size() == 2);
    {
      REQUIRE(std::string_view{e[0].sourceLoc.func} == "InnerFunc");
      MessageWalker wlk{e[0].messages};
      REQUIRE(wlk.Has("cstr", "logging some predefined string"));
      REQUIRE(wlk.Has("uint", "1337"));
      REQUIRE(wlk.Has("int", "-1337"));
      REQUIRE(wlk.Has("custom", "captured a vector that has 3 elements"));
      REQUIRE(wlk.Has("custom", "vector has 4 elements; this one won't expire after Orphan but will expire after exception"));
      REQUIRE(wlk.Has("custom", "owned, will be copied"));
      REQUIRE(wlk.Has("custom", expiredMessage));
      REQUIRE(wlk.HasStacktrace());
      REQUIRE(wlk.MatchedAll());
    }
    {
      REQUIRE(std::string_view{e[1].sourceLoc.func} == "OuterFunc");
      MessageWalker wlk{e[1].messages};
      REQUIRE(wlk.Has("custom", "some message from the outer scope just to verify proper capture of the whole chain"));
      REQUIRE(wlk.MatchedAll());
    }
  }

  REQUIRE_FALSE(Antigo::HasExceptionWitnessOrphan());
  REQUIRE_FALSE(Antigo::HasExceptionWitness());

  REQUIRE(f.evaluated1 == 2);
  REQUIRE(f.evaluated2 == 1);
  REQUIRE(f.evaluated3 == 2);
  REQUIRE(f.evaluated4 == 0);
  REQUIRE(f.evaluated5 == 2);

  REQUIRE(Antigo::impl::HasCleanState());
}

TEST_CASE("Test context too many messages")
{
  static constexpr size_t kMaxDataFrameCnt = 60;
  struct {
    void Func(size_t extra) {
      ANTIGO_CONTEXT_INIT(ctx);

      for (size_t i = 0; i < kMaxDataFrameCnt + extra; ++i) {
        ctx.AddUnsigned(i);
      }
      ctx.Orphan();
    }
  } f;

  {
    f.Func(4);
    REQUIRE(Antigo::HasExceptionWitnessOrphan());
    auto w = Antigo::PopExceptionWitnessOrphan();

    auto& e = w.entries;
    REQUIRE(e.size() == 1);

    MessageWalker wlk{e[0].messages};
    for (size_t i = 0; i < kMaxDataFrameCnt; ++i) {
      REQUIRE(wlk.Has("uint", std::to_string(i)));
    }
    REQUIRE(wlk.Has("meta", "4 last messages didn't fit into buffer"));
    REQUIRE(wlk.HasStacktrace());
    REQUIRE(wlk.MatchedAll());

    REQUIRE_FALSE(Antigo::HasExceptionWitnessOrphan());
    REQUIRE(Antigo::impl::HasCleanState());
  }

  {
    f.Func(1337);
    REQUIRE(Antigo::HasExceptionWitnessOrphan());
    auto w = Antigo::PopExceptionWitnessOrphan();

    auto& e = w.entries;
    REQUIRE(e.size() == 1);

    MessageWalker wlk{e[0].messages};
    for (size_t i = 0; i < kMaxDataFrameCnt; ++i) {
      REQUIRE(wlk.Has("uint", std::to_string(i)));
    }
    REQUIRE(wlk.Has("meta", "255+ last messages didn't fit into buffer"));
    REQUIRE(wlk.HasStacktrace());
    REQUIRE(wlk.MatchedAll());

    REQUIRE_FALSE(Antigo::HasExceptionWitnessOrphan());
    REQUIRE(Antigo::impl::HasCleanState());
  }

  REQUIRE(Antigo::impl::HasCleanState());
}

TEST_CASE("Test exception during exception handling")
{
  struct {
    struct InnerStruct2 {
      ~InnerStruct2() {
        ANTIGO_CONTEXT_INIT(ctx); // 2 -> 2
        try {
          throw CustomTestException();
        } catch (const CustomTestException&) {
          // ignore
        }
      }
    };

    struct InnerStruct {
      ~InnerStruct() {
        ANTIGO_CONTEXT_INIT(ctx); // 1 -> 1
        try {
          InnerStruct2 is2;
          throw CustomTestException();
        } catch (const CustomTestException&) {
          // ignore
        }
      }
    };

    void Func() {
      ANTIGO_CONTEXT_INIT(ctx); // 0 -> 0
      try {
        InnerStruct is;
        throw CustomTestException();
      } catch (const CustomTestException&) {
        // ignore
      }
    }
  } f;

  f.Func();

  REQUIRE(Antigo::HasExceptionWitnessOrphan());
  auto w = Antigo::PopExceptionWitnessOrphan();
  REQUIRE(w.reason == "exception");
  REQUIRE(w.entries.size() == 3);
  REQUIRE(w.entries[0].sourceLoc.func == std::string_view{"~InnerStruct2"});
  REQUIRE(w.entries[1].sourceLoc.func == std::string_view{"~InnerStruct"});
  REQUIRE(w.entries[2].sourceLoc.func == std::string_view{"Func"});

  // for now, it would only return one witness, i.e. ~InnerStruct() and Func() wouldn't have a separate one, even though they have separate exceptions

  REQUIRE(Antigo::impl::HasCleanState());
}

TEST_CASE("Test LogInnerExecution")
{
  static constexpr size_t kMaxDataFrameCnt = 60;
  struct {
    int assertFailedDepth = -1;

    void Func(size_t depth) {
      ANTIGO_CONTEXT_INIT(ctx);
      if (!ctx.IsLoggingInnerExecution()) {
        assertFailedDepth = depth;
        return;
      }
      if (depth == 0) {
        return;
      }
      ctx.AddUnsigned(depth);
      Func(depth - 1);
    }

    Antigo::ResolvedContext Entry() {
      ANTIGO_CONTEXT_INIT(ctx);
      ctx.LogInnerExecution();
      Func(3);
      return ctx.Resolve();
    }
  } f;

  auto w = f.Entry();
  w.Print();
  REQUIRE_FALSE(Antigo::HasExceptionWitnessOrphan());
  REQUIRE(f.assertFailedDepth == -1);
}

// XXX test if catch can receive an unrelated context (probably can if we throw and catch in-place with orphan/detached already existing prior to that)

// XXX uncleaned witnesses can probably stick to a thread pool local storage
// XXX 20250103 0021: probably is only problem for orphans. witnesses don't live longer than the current context does

// XXX uncaught exceptions might inc/dec between ctor and dtor and return back - is it a problem?
// inc-dec по идее это будет значить либо что мы поймали эксепшн, который возник выше этого контекста - т.е. он остался изолированным в текущем скоупе
// dec-inc - это будет значить, что текущий скоуп как-то погасил эксепшн из фрейма ниже. возможно ли это вообще?
// TODO: почекать, в каких ситуациях uncaught_exceptions > 1
