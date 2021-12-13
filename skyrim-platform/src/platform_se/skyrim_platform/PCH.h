#pragma once

#define WIN32_LEAN_AND_MEAN // disable mitigations
#define NOMINMAX            // fixes std::numeric::min()/max() warning

#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include <Windows.h>
#include <tlhelp32.h>
//#include <cstdlib>

#include <spdlog/sinks/basic_file_sink.h>

#include <common/IDebugLog.h>
#include <common/ITypes.h>
#include <skse64/GameData.h>
#include <skse64/GameFormComponents.h>
#include <skse64/NiRenderer.h> // used in main.cpp BSRenderManager

namespace logger = SKSE::log;

using namespace std::literals;

using IVM = RE::BSScript::IVirtualMachine;
using VM = RE::BSScript::Internal::VirtualMachine;
using StackID = RE::VMStackID;
using Variable = RE::BSScript::Variable;
using FixedString = RE::BSFixedString;

#define DLLEXPORT __declspec(dllexport)
