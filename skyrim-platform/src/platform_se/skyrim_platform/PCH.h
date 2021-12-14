#pragma once

#define WIN32_LEAN_AND_MEAN // disable mitigations
#define NOMINMAX            // fixes std::numeric::min()/max() warning

#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include <Windows.h>
#include <tlhelp32.h>

#include <spdlog/sinks/basic_file_sink.h>

namespace logger = SKSE::log;

using namespace std::literals;

using IVM = RE::BSScript::IVirtualMachine;
using VM = RE::BSScript::Internal::VirtualMachine;
using StackID = RE::VMStackID;
using Variable = RE::BSScript::Variable;
using FixedString = RE::BSFixedString;

#define COLOR_ALPHA(in) ((in >> 24) & 0xFF)
#define COLOR_RED(in) ((in >> 16) & 0xFF)
#define COLOR_GREEN(in) ((in >> 8) & 0xFF)
#define COLOR_BLUE(in) ((in >> 0) & 0xFF)

#define DLLEXPORT __declspec(dllexport)
