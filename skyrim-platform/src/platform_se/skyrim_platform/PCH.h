#pragma once

/* disable unused headers from Windows.h */
#define WIN32_LEAN_AND_MEAN
#define NOSOUND          // Sound driver routines
#define NOTEXTMETRIC     // typedef TEXTMETRIC and associated routines
#define NOWH             // SetWindowsHook and WH_*
#define NOWINOFFSETS     // GWL_*, GCL_*, associated routines
#define NOCOMM           // COMM driver routines
#define NOKANJI          // Kanji support stuff.
#define NOHELP           // Help engine interface.
#define NOPROFILER       // Profiler interface.
#define NODEFERWINDOWPOS // DeferWindowPos routines
#define NOMCX            // Modem Configuration Extensions
#define NOSYSMETRICS     // SM_*
#define NOMENUS          // MF_*
#define NOICONS          // IDI_*
#define NOKEYSTATES      // MK_*
#define NOSYSCOMMANDS    // SC_*
#define NORASTEROPS      // Binary and Tertiary raster ops
#define OEMRESOURCE      // OEM Resource values
#define NOATOM           // Atom Manager routines
#define NOCLIPBOARD      // Clipboard routines
#define NOCOLOR          // Screen colors
#define NODRAWTEXT       // DrawText() and DT_*
#define NOGDI            // All GDI defines and routines
#define NOKERNEL         // All KERNEL defines and routines
#define NOMEMMGR         // GMEM_*, LMEM_*, GHND, LHND, associated routines
#define NOMETAFILE       // typedef METAFILEPICT
#define NOMINMAX         // Macros min(a,b) and max(a,b)
#define NOOPENFILE       // OpenFile(), OemToAnsi, AnsiToOem, and OF_*
#define NOSCROLL         // SB_* and scrolling routines
#define NOSERVICE // All Service Controller routines, SERVICE_ equates, etc.

#include <RE/Skyrim.h>
#include <REL/Relocation.h>
#include <SKSE/SKSE.h>

#include <codecvt>
#include <concepts>
#include <condition_variable>
#include <iostream> // savefile

#include <comdef.h> // _bstr_t
#include <dxgi.h>
#include <shellapi.h>
#include <shlobj_core.h>
#include <stringapiset.h>
#include <tlhelp32.h>

#include <SimpleIni.h>
#include <asio/executor_work_guard.hpp>
#include <asio/io_context.hpp>
#include <asio/post.hpp>
#include <cef_values.h>
#include <cmrc/cmrc.hpp>
#include <core_library/Meta.hpp>
#include <frida-gum.h>
#include <hooks/D3D11Hook.hpp>
#include <hooks/DInputHook.hpp>
#include <hooks/IInputListener.h>
#include <hooks/WindowsHook.hpp>
#include <internal/cef_ptr.h>
#include <internal/cef_types.h>
#include <nlohmann/json.hpp>
#include <reverse/App.hpp>
#include <reverse/AutoPtr.hpp>
#include <reverse/Entry.hpp>
#include <robin_hood.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <ui/DX11RenderHandler.h>
#include <ui/MyChromiumApp.h>
#include <ui/MyRenderHandler.h>
#include <ui/ProcessMessageListener.h>
#include <ui/TextToDraw.h>
#include <zlib.h>

namespace logger = SKSE::log;
namespace stl = SKSE::stl;

using namespace std::literals;

using IVM = RE::BSScript::IVirtualMachine;
using VM = RE::BSScript::Internal::VirtualMachine;
using StackID = RE::VMStackID;
using Variable = RE::BSScript::Variable;
using FixedString = RE::BSFixedString;
using TypeInfo = RE::BSScript::TypeInfo;

#define COLOR_ALPHA(in) ((in >> 24) & 0xFF)
#define COLOR_RED(in) ((in >> 16) & 0xFF)
#define COLOR_GREEN(in) ((in >> 8) & 0xFF)
#define COLOR_BLUE(in) ((in >> 0) & 0xFF)

#define DLLEXPORT __declspec(dllexport)
#define NOINLINE __declspec(noinline)

template <typename E>
constexpr typename std::underlying_type<E>::type to_underlying(E e) noexcept
{
  return static_cast<typename std::underlying_type<E>::type>(e);
}

// this should be moved somewhere.
// use only when RE::malloc is used for memory allocation
template <typename T>
struct game_type_pointer_deleter
{
  void operator()(T* ptr) { RE::free((void*)ptr); }
};

#include "JsEngine.h" // imports TaskQueue.h
#include "Version.h"

#include "game/Offsets.h"

#include "game/Classes.h"
#include "game/Events.h"

#include "Concepts.h"
