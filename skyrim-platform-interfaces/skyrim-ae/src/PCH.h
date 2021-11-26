#pragma once

#define WIN32_LEAN_AND_MEAN // disable mitigations
#define NOMINMAX            // fixes std::numeric::min()/max() warning

#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include <spdlog/sinks/basic_file_sink.h>

namespace logger = SKSE::log;

using namespace std::literals;

#define DLLEXPORT __declspec(dllexport)

#include "Version.h"
