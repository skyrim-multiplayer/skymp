#pragma once
#include <cef/core_library/Meta.hpp>

struct OverlayService;

struct InputService
{
  InputService(OverlayService& aOverlay) noexcept;
  ~InputService() noexcept;

  TP_NOCOPYMOVE(InputService);
};