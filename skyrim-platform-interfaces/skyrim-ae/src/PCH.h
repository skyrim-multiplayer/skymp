#pragma once

#define WIN32_LEAN_AND_MEAN // disable mitigations
#define NOMINMAX            // fixes std::numeric::min()/max() warning

#include <stdarg.h>

#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include <spdlog/sinks/basic_file_sink.h>

/**
 * skse64 references
 *
 * -> TintMask class is not implemented in CommonLib
 * -> *g_invalidRefHandle
 * -> used by TESModPlatform
 *
 * .. to be continued
 *
 */
#include <common/ITypes.h>
#include <skse64/GameObjects.h>
#include <skse64/GameReferences.h>

namespace logger = SKSE::log;

using namespace std::literals;

using VM = RE::BSScript::Internal::VirtualMachine;
using IVM = RE::BSScript::IVirtualMachine;
using StackID = RE::VMStackID;
using FixedString = RE::BSFixedString;
using Variable = RE::BSScript::Variable;

#define DLLEXPORT __declspec(dllexport)

#include "Version.h"
#include "frida-gum.h"
