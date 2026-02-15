#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>

#include "CombineBrowser.h"
#include "GroupStack.h"
#include "GroupUtils.h"
#include "IdMapping.h"
#include "espm.h"

namespace espm {

class Browser;
class RecordHeader;
class Combiner;

class Combiner
{
public:
  Combiner();
  ~Combiner();

  class CombineError : public std::logic_error
  {
  public:
    CombineError(const std::string& str)
      : logic_error(str){};
  };

  void AddSource(Browser* src, const char* fileName) noexcept;

  // Throws CombineError
  std::unique_ptr<CombineBrowser> Combine();

private:
  std::shared_ptr<CombineBrowser::Impl> pImpl;

  Combiner(const Combiner&) = delete;
  void operator=(const Combiner&) = delete;
};

}
