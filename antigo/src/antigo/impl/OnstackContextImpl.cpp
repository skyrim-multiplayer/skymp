#include <cassert>

#include <cpptrace/basic.hpp>
#include <cpptrace/from_current.hpp>
#include <exception>
#include <iostream>
#include <memory>
#include <sstream>
#include <thread>

#include "antigo/Context.h"
#include "antigo/impl/OnstackContextImpl.h"
#include "antigo/ResolvedContext.h"
#include "antigo/impl/ExecutionData.h"

#ifndef WIN32
#define ANTIGO_TRY CPPTRACE_TRY
#define ANTIGO_CATCH CPPTRACE_CATCH
#else
#define ANTIGO_TRY try
#define ANTIGO_CATCH catch
#endif

namespace Antigo {

namespace {
std::string ToString(const std::vector<InnerExecutionEvent>& evts) {
  constexpr size_t maxToPrint = 30;
  size_t start = 0;
  if (start + maxToPrint < evts.size()) {
    start = evts.size() - maxToPrint;
  }
  std::stringstream ss;
  ss << "inner tracer messages [" << start << ".." << evts.size() << ") - latest exits = earliest enter-s last\n";
  // for (size_t ip1 = evts.size(); ip1 > start; --ip1) {
  //   size_t i = ip1 - 1;
  for (size_t i = start; i < evts.size(); ++i) {
    ss << evts[i].resolvedCtxEntry << "\n";
    ss << "[" << i << "] = " << evts[i].type << " ^";
    // if (i != start) {
    if (evts[i].type == "enter") {
      ss << ">>>>>>>>>";
    }
    if (evts[i].type == "leave") {
      ss << "<<<<<<<<<";
    }
    if (i + 1 != evts.size()) {
      ss << "\n";
    }
  }
  return std::move(ss).str();
}
} // namespace

OnstackContextImpl::OnstackContextImpl(const char* filename_, size_t linenum_, const char* funcname_): h{}, dataFrames{} {
  h.filename = filename_;
  h.linenum = std::min<size_t>(linenum_, std::numeric_limits<decltype(h.linenum)>::max());
  h.dataFramesCnt = 0;
  h.skippedDataFramesCnt = 0;
  h.destructing = false;
  h.errorOnTop = false;
  h.funcname = funcname_;

  h.downCtx = nullptr;
  if (GetCurrentExecutionData().stackCtxChain.size()) {
    assert(GetCurrentExecutionData().stackCtxChain.back());
    h.downCtx = GetCurrentExecutionData().stackCtxChain.back();
  }

  h.slow = nullptr;

  // h.rawTrace = cpptrace::generate_raw_trace(); // XXX disable for emscripten (can't build)
  h.uncaughtExceptions = std::uncaught_exceptions();

  if (h.uncaughtExceptions) {
    AddMessage("ctx: has uncaught exceptions; count=");
    AddUnsigned(h.uncaughtExceptions);
  }

  GetCurrentExecutionData().stackCtxChain.push_back(this);

  if (h.downCtx != nullptr && h.downCtx->h.slow != nullptr) {
    auto& evts = AccessSlow().innerExecEvts = h.downCtx->h.slow->innerExecEvts;
    // evts->push_back({h.filename, h.linenum, h.funcname, "enter"});
    evts->push_back({"enter", ResolveCurrentImpl()});
  }

  // if (h.downCtx == nullptr) {
  //   // 20241227: if debug include message thread id
  //   std::this_thread::get_id();
  // }

  // 20241218 1: push_back & pop_back for reserve storage and noexcept?
  // next step: common buffer for prepared message frames?
  // exception safety: cancel adding in case of error; also skip all dtor logic in that case

  // 20241227 evg: self-trace - писать в цикличный буфер в таком же +- формате в локальный сторадж потока + в себя же
}

OnstackContextImpl::~OnstackContextImpl() {
  ANTIGO_TRY {
    h.destructing = true;

    // if (!(++GetCurrentExecutionData().ticker & 0xffff)) {
    //   GetCurrentExecutionData().orphans.emplace_back(ResolveCtxStackImpl("ticker"));
    // }

    if (h.slow != nullptr) {
      auto& evts = h.slow->innerExecEvts;
      if (h.downCtx == nullptr || h.downCtx->h.slow == nullptr || h.downCtx->h.slow->innerExecEvts == nullptr) {
        // stop
      } else {
        evts->push_back({"leave", ResolveCurrentImpl()});
      }
    }

    assert(GetCurrentExecutionData().stackCtxChain.size() && GetCurrentExecutionData().stackCtxChain.back() == this);
    if (std::uncaught_exceptions() != h.uncaughtExceptions || (h.downCtx && h.downCtx->h.uncaughtExceptions != h.uncaughtExceptions)) {
      auto& w = GetCurrentExecutionData().errorWitnesses;
      if (!h.errorOnTop) {
        // XXX 20250112 1508 related: условие здесь должно как-то учитывать, что мы могли провалиться сверху с другим слоем эксепшена. или пофиг?)
        ANTIGO_TRY {
          w.emplace_back(ResolveCtxStackImpl("exception"));
        } ANTIGO_CATCH (const std::exception& e) {
          std::cerr << "dtor: context resolved with an exception, what: " << e.what() << "\n";
          cpptrace::from_current_exception().print();
        }
      }
      if (h.downCtx == nullptr) {
        while (!w.empty()) {
          GetCurrentExecutionData().orphans.push_back(std::move(w.back()));
          w.pop_back();
        }
      } else {
        h.downCtx->h.errorOnTop = true;
      }
    } else {
      auto& w = GetCurrentExecutionData().errorWitnesses;
      while (!w.empty()) {
        GetCurrentExecutionData().orphans.push_back(std::move(w.back()));
        w.pop_back();
      }
    }
    GetCurrentExecutionData().stackCtxChain.pop_back();
  } ANTIGO_CATCH (const std::exception& e) {
    std::cerr << "fatal exception, what: " << e.what() << "\n";
    cpptrace::from_current_exception().print();
    std::terminate();
    // terminating here on purpose, our state could get inconsistent
  }
}

ResolvedContextEntry OnstackContextImpl::ResolveCurrentImpl() const {
  ResolvedContextEntry entry;
  entry.sourceLoc.filename = h.filename;
  entry.sourceLoc.line = h.linenum;
  entry.sourceLoc.func = h.funcname;

  for (size_t i = 0; i < h.dataFramesCnt; ++i) {
    entry.messages.emplace_back(dataFrames[i].Resolve());
  }

  if (h.skippedDataFramesCnt) {
    std::string msg = std::to_string(h.skippedDataFramesCnt);
    if (h.skippedDataFramesCnt == std::numeric_limits<decltype(h.skippedDataFramesCnt)>::max()) {
      msg += "+";
    }
    msg += " last messages didn't fit into buffer";
    entry.messages.push_back(ResolvedMessageEntry{"meta", std::move(msg)});
  }
  return entry;
}

ResolvedContext OnstackContextImpl::ResolveCtxStackImpl(std::string reason) const {
  ResolvedContext result;
  result.reason = std::move(reason);
  const auto& chain = GetCurrentExecutionData().stackCtxChain;
  for (size_t i = chain.size(); i > 0; --i) {
    result.entries.push_back(chain[i - 1]->ResolveCurrentImpl());
  }
  if (!result.entries.empty()) {
    result.entries[0].messages.push_back({"stacktrace", cpptrace::generate_trace().to_string()});
    if (h.slow != nullptr && h.slow->innerExecEvts != nullptr) {
      result.entries[0].messages.push_back({"inner_tracer", ToString(*h.slow->innerExecEvts)});
    }
    // result.entries[0].messages.push_back({"resolution_id", "TODO currenttime.rand"});
  }
  return result;
}

ResolvedContext OnstackContextImpl::Resolve() const {
  return ResolveCtxStackImpl("ondemand");
}

void OnstackContextImpl::Orphan() const {
  GetCurrentExecutionData().orphans.emplace_back(Resolve());
}

void OnstackContextImpl::LogInnerExecution() {
  auto& slow = AccessSlow();
  slow.innerExecEvts = std::make_shared<std::vector<InnerExecutionEvent>>();

  // AddLambdaWithOwned([evts = slow.innerExecEvts] {
  //   return ToString(*evts);
  // });
}

bool OnstackContextImpl::IsLoggingInnerExecution() const {
  return h.slow != nullptr && h.slow->innerExecEvts != nullptr;
}

}
