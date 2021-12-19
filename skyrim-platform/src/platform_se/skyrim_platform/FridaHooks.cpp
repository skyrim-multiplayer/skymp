/*
 * To build, set up your Release configuration like this:
 *
 * [Runtime Library]
 * Multi-threaded (/MT)
 *
 * Visit www.frida.re to learn more about Frida.
 */

#include <frida/frida-gum.h>

#include "EventsApi.h"
#include "FridaHooksUtils.h"
#include "PapyrusTESModPlatform.h"
#include "StringHolder.h"
#include <RE/ConsoleLog.h>
#include <RE/TESObjectREFR.h>
#include <skse64/GameData.h>
#include <skse64/GameTypes.h>

#include <RE/BSScript/Object.h>
#include <RE/BSScript/ObjectTypeInfo.h>
#include <RE/BSScript/Variable.h>
#include <RE/SkyrimScript/HandlePolicy.h>
//#include "VmFunctionArguments.h"
//#include <RE/BSScript/IFunctionArguments.h>
#include "CallNative.h"

#include "hooks/DInputHook.hpp"
#include "ui/DX11RenderHandler.h"
#include <algorithm>
#include <sstream>
#include <windows.h>

typedef struct _ExampleListener ExampleListener;
typedef enum _ExampleHookId ExampleHookId;

struct _ExampleListener
{
  GObject parent;
};

enum _ExampleHookId
{
  HOOK_SEND_ANIMATION_EVENT,
  DRAW_SHEATHE_WEAPON_ACTOR,
  DRAW_SHEATHE_WEAPON_PC,
  QUEUE_NINODE_UPDATE,
  APPLY_MASKS_TO_RENDER_TARGET,
  RENDER_CURSOR_MENU,
  SEND_EVENT,
  SEND_EVENT_ALL,
  CONSOLE_VPRINT,
  MAGIC_EFFECT_APPLY
};

static void example_listener_iface_init(gpointer g_iface, gpointer iface_data);

#define EXAMPLE_TYPE_LISTENER (example_listener_get_type())
G_DECLARE_FINAL_TYPE(ExampleListener, example_listener, EXAMPLE, LISTENER,
                     GObject)
G_DEFINE_TYPE_EXTENDED(ExampleListener, example_listener, G_TYPE_OBJECT, 0,
                       G_IMPLEMENT_INTERFACE(GUM_TYPE_INVOCATION_LISTENER,
                                             example_listener_iface_init))

namespace {
class InterceptorWrapper
{
public:
  InterceptorWrapper(GumInterceptor* interceptor_)
    : interceptor(interceptor_)
  {
    gum_interceptor_begin_transaction(interceptor);
  }

  ~InterceptorWrapper() { gum_interceptor_end_transaction(interceptor); }

  void Attach(GumInvocationListener* listener, int offset,
              _ExampleHookId hookId)
  {
    int r = gum_interceptor_attach(interceptor,
                                   (void*)(REL::Module::BaseAddr() + offset),
                                   listener, GSIZE_TO_POINTER(hookId));

    if (GUM_ATTACH_OK != r) {
      char buf[1025];
      sprintf_s(buf, "Interceptor failed with %d for hook %d", int(r),
                int(hookId));
      MessageBox(0, buf, "Error", MB_ICONERROR);
      return;
    }
  }

private:
  GumInterceptor* const interceptor;
};
}

void SetupFridaHooks()
{
  GumInterceptor* interceptor;

  gum_init_embedded();

  InterceptorWrapper w(gum_interceptor_obtain());
  auto listener =
    (GumInvocationListener*)g_object_new(EXAMPLE_TYPE_LISTENER, NULL);

  w.Attach(listener, 6353472, HOOK_SEND_ANIMATION_EVENT);
  w.Attach(listener, 6104992, DRAW_SHEATHE_WEAPON_ACTOR);
  w.Attach(listener, 7141008, DRAW_SHEATHE_WEAPON_PC);
  w.Attach(listener, 6893840, QUEUE_NINODE_UPDATE);
  w.Attach(listener, 4043808, APPLY_MASKS_TO_RENDER_TARGET);
  w.Attach(listener, 5367792, RENDER_CURSOR_MENU);
  w.Attach(listener, 19244800, SEND_EVENT);
  w.Attach(listener, 19245744, SEND_EVENT_ALL);
  w.Attach(listener, 8766499, CONSOLE_VPRINT);
  w.Attach(listener, 9666192, MAGIC_EFFECT_APPLY);
}

class VariableAccess : public RE::BSScript::Variable
{
public:
  VariableAccess() = delete;

  static RE::BSScript::TypeInfo GetType(const RE::BSScript::Variable& v)
  {
    return ((VariableAccess&)v).varType;
  }

  static RE::BSTSmartPointer<RE::BSScript::Object> GetObjectSmartPtr(
    const RE::BSScript::Variable& v)
  {
    return ((VariableAccess&)v).value.obj;
  }
};

thread_local uint32_t g_queueNiNodeActorId = 0;

bool g_allowHideCursorMenu = true;
bool g_transparentCursor = false;

thread_local gpointer g_eventArgsPointer = nullptr;
thread_local char* g_eventName = nullptr;
thread_local uint32_t g_eventSelfId = 0;
std::vector<int> g_argsOffsets;

static void example_listener_on_enter(GumInvocationListener* listener,
                                      GumInvocationContext* ic)
{
  ExampleListener* self = EXAMPLE_LISTENER(listener);
  auto hook_id = gum_invocation_context_get_listener_function_data(ic);

  auto _ic = (_GumInvocationContext*)ic;

  switch ((size_t)hook_id) {
    case CONSOLE_VPRINT: {
      char* refr =
        _ic->cpu_context->rdx ? (char*)_ic->cpu_context->rdx : nullptr;

      if (!refr) {
        return;
      }

      EventsApi::SendConsoleMsgEvent(refr);

      break;
    }
    case SEND_EVENT: {
      int argIdx = 2;

      auto eventName =
        (char**)gum_invocation_context_get_nth_argument(ic, argIdx);
      g_eventName = *eventName;

      auto handle =
        (RE::VMHandle)gum_invocation_context_get_nth_argument(ic, 1);
      auto args = static_cast<RE::BSScript::IFunctionArguments*>(
        gum_invocation_context_get_nth_argument(ic, 3));

      auto vm = RE::BSScript::Internal::VirtualMachine::GetSingleton();
      bool blockEvents = TESModPlatform::GetPapyrusEventsBlocked();

      uint32_t selfId = 0;

      auto policy = vm->GetObjectHandlePolicy();
      if (policy) {
        if (auto actor = policy->GetObjectForHandle(
              RE::FormType::ActorCharacter, handle)) {
          selfId = actor->GetFormID();
        }
        if (auto refr =
              policy->GetObjectForHandle(RE::FormType::Reference, handle)) {
          selfId = refr->GetFormID();
        }
      }

      g_eventSelfId = selfId;
      auto offset = FridaHooksUtils::GetNthVtableElement(args, 0, 1);

      if (auto c = RE::ConsoleLog::GetSingleton()) {
        if (std::find(g_argsOffsets.begin(), g_argsOffsets.end(), offset) !=
            g_argsOffsets.end()) {
           //c->Print("Offset %i already added", offset);
          if (strcmp(*eventName, "OnMagicEffectApply") == 0) {
            //c->Print("MagicEffect EVENT already here! offset: %i", offset);
          }
        } else {
          g_argsOffsets.push_back(offset);
          InterceptorWrapper w(gum_interceptor_obtain());
          auto listener =
            (GumInvocationListener*)g_object_new(EXAMPLE_TYPE_LISTENER, NULL);
          w.Attach(listener, offset,
                   static_cast<ExampleHookId>(100 + g_argsOffsets.size()));
          c->Print("New offset added %i, total: %i, hookid: %i", offset,
                   g_argsOffsets.size(), (100 + g_argsOffsets.size()));
        }
      }

      /*if (strcmp(*eventName, "OnMagicEffectApply") == 0) {
        if (auto c = RE::ConsoleLog::GetSingleton()) {
          auto offset0 = FridaHooksUtils::GetNthVtableElement(args, 0, 0);
          auto offset1 = FridaHooksUtils::GetNthVtableElement(args, 0, 1);
          auto offset2 = FridaHooksUtils::GetNthVtableElement(args, 0, 2);
          auto offset3 = FridaHooksUtils::GetNthVtableElement(args, 0, 3);
          auto offset4 = FridaHooksUtils::GetNthVtableElement(args, 0, 4);
          
          c->Print("EVENT! name: %s, selfid: %i, offset0: %i, offset1: %i, "
                   "offset2: %i, offset3: %i, offset4: %i,",
                   *eventName, selfId, offset0, offset1, offset2, offset3, offset4);
        }
      }*/

      // std::string eventNameStr = *eventName;
      // EventsApi::SendPapyrusEventEnter(selfId, eventNameStr);

      if (strcmp(*eventName, "OnUpdate") != 0 && vm) {
        vm->attachedScriptsLock.Lock();
        auto it = vm->attachedScripts.find(handle);

        if (it != vm->attachedScripts.end()) {
          auto& scripts = it->second;

          for (size_t i = 0; i < scripts.size(); i++) {
            auto script = scripts[i].get();
            auto info = script->GetTypeInfo();
            auto name = info->GetName();

            const char* skyui_name = "SKI_"; // start skyui object name

            // RE::ConsoleLog::GetSingleton()->Print(name);

            if (strlen(name) >= 4 && name[0] == skyui_name[0] &&
                name[1] == skyui_name[1] && name[2] == skyui_name[2] &&
                name[3] == skyui_name[3]) {
              blockEvents = false;
              break;
            }
          }
        }

        vm->attachedScriptsLock.Unlock();
      }

      if (blockEvents) {
        static const auto fsEmpty = new RE::BSFixedString("");
        gum_invocation_context_replace_nth_argument(ic, argIdx, fsEmpty);
      }
      break;
    }
    case MAGIC_EFFECT_APPLY : {
      if (auto c = RE::ConsoleLog::GetSingleton()) {
        c->Print("Magic Effect Apply ENTER");
      }
      break;
    }
    case SEND_EVENT_ALL:
      if (auto c = RE::ConsoleLog::GetSingleton())
        c->Print("SendEventAll");
      break;
    case DRAW_SHEATHE_WEAPON_PC: {
      auto refr =
        _ic->cpu_context->rcx ? (RE::Actor*)(_ic->cpu_context->rcx) : nullptr;
      uint32_t formId = refr ? refr->formID : 0;

      union
      {
        size_t draw;
        uint8_t byte[8];
      };

      draw = (size_t)gum_invocation_context_get_nth_argument(ic, 1);

      auto falseValue = gpointer(*byte ? draw - 1 : draw);
      auto trueValue = gpointer(*byte ? draw : draw + 1);

      auto mode = TESModPlatform::GetWeapDrawnMode(formId);
      if (mode == TESModPlatform::WEAP_DRAWN_MODE_ALWAYS_TRUE) {
        gum_invocation_context_replace_nth_argument(ic, 1, trueValue);
      } else if (mode == TESModPlatform::WEAP_DRAWN_MODE_ALWAYS_FALSE) {
        gum_invocation_context_replace_nth_argument(ic, 1, falseValue);
      }
      break;
    }
    case DRAW_SHEATHE_WEAPON_ACTOR: {
      auto refr =
        _ic->cpu_context->rcx ? (RE::Actor*)(_ic->cpu_context->rcx) : nullptr;
      uint32_t formId = refr ? refr->formID : 0;

      auto draw = (uint32_t*)gum_invocation_context_get_nth_argument(ic, 1);

      auto mode = TESModPlatform::GetWeapDrawnMode(formId);
      if (mode == TESModPlatform::WEAP_DRAWN_MODE_ALWAYS_TRUE) {
        gum_invocation_context_replace_nth_argument(ic, 1, gpointer(1));
      } else if (mode == TESModPlatform::WEAP_DRAWN_MODE_ALWAYS_FALSE) {
        gum_invocation_context_replace_nth_argument(ic, 1, gpointer(0));
      }
      break;
    }
    case HOOK_SEND_ANIMATION_EVENT: {
      auto refr = _ic->cpu_context->rcx
        ? (RE::TESObjectREFR*)(_ic->cpu_context->rcx - 0x38)
        : nullptr;
      uint32_t formId = refr ? refr->formID : 0;

      constexpr int argIdx = 1;
      auto animEventName =
        (char**)gum_invocation_context_get_nth_argument(ic, argIdx);

      if (!refr || !animEventName)
        break;

      std::string str = *animEventName;
      EventsApi::SendAnimationEventEnter(formId, str);
      if (str != *animEventName) {
        auto fs = const_cast<RE::BSFixedString*>(
          &StringHolder::ThreadSingleton()[str]);
        auto newAnimEventName = reinterpret_cast<char**>(fs);
        gum_invocation_context_replace_nth_argument(ic, argIdx,
                                                    newAnimEventName);
      }
      break;
    }
    case QUEUE_NINODE_UPDATE: {
      auto refr = _ic->cpu_context->rcx
        ? (RE::TESObjectREFR*)(_ic->cpu_context->rcx)
        : nullptr;

      uint32_t id = refr ? refr->formID : 0;

      g_queueNiNodeActorId = id;
      break;
    }
    case APPLY_MASKS_TO_RENDER_TARGET: {
      if (g_queueNiNodeActorId > 0) {
        auto tints = TESModPlatform::GetTintsFor(g_queueNiNodeActorId);
        if (tints) {
          gum_invocation_context_replace_nth_argument(ic, 0, tints.get());
        }
      }

      g_queueNiNodeActorId = 0;
      break;
    }
    case RENDER_CURSOR_MENU: {
      static auto fsCursorMenu = new BSFixedString("Cursor Menu");
      auto cursorMenu = FridaHooksUtils::GetMenuByName(fsCursorMenu);
      auto this_ = (int64_t*)_ic->cpu_context->rcx;
      if (g_allowHideCursorMenu) {
        if (this_) {
          if (cursorMenu == this_) {
            bool kRunningAE = false;
            if (kRunningAE) {
              FridaHooksUtils::SaveCursorPosition();
            }
            bool& visibleFlag = CEFUtils::DX11RenderHandler::Visible();
            bool& focusFlag = CEFUtils::DInputHook::ChromeFocus();
            if (visibleFlag && focusFlag) {
              if (!g_transparentCursor) {
                if (FridaHooksUtils::SetMenuNumberVariable(
                      fsCursorMenu, "_root.mc_Cursor._alpha", 0)) {
                  g_transparentCursor = true;
                }
              }
            } else {
              if (g_transparentCursor) {
                if (FridaHooksUtils::SetMenuNumberVariable(
                      fsCursorMenu, "_root.mc_Cursor._alpha", 100)) {
                  g_transparentCursor = false;
                }
              }
            }
          }
        }
      }
      break;
    }
    default: {
      if (reinterpret_cast<size_t>(hook_id) > 100 &&
          reinterpret_cast<size_t>(hook_id) <= (100 + g_argsOffsets.size())) {
        g_eventArgsPointer = gum_invocation_context_get_nth_argument(ic, 1);

        if (auto c = RE::ConsoleLog::GetSingleton()) {
          // c->Print("Operator Enter: %i", reinterpret_cast<int>(hook_id));
        }
      }
    }
  }
}

static void example_listener_on_leave(GumInvocationListener* listener,
                                      GumInvocationContext* ic)
{
  ExampleListener* self = EXAMPLE_LISTENER(listener);
  auto hook_id = gum_invocation_context_get_listener_function_data(ic);

  switch ((size_t)hook_id) {
    case HOOK_SEND_ANIMATION_EVENT: {
      bool res = !!gum_invocation_context_get_return_value(ic);
      EventsApi::SendAnimationEventLeave(res);
      break;
    }
    case SEND_EVENT: {
      // EventsApi::SendPapyrusEventLeave();
      break;
    }
    case MAGIC_EFFECT_APPLY: {
      if (auto c = RE::ConsoleLog::GetSingleton()) {
        c->Print("Magic Effect Apply LEAvLE");
      }
      break;
    }
    case DRAW_SHEATHE_WEAPON_ACTOR: {
      g_eventName = nullptr;
      break;
    }
    case RENDER_CURSOR_MENU: {
      g_eventName = nullptr;
      break;
    }
    default: {
      if (auto c = RE::ConsoleLog::GetSingleton()) {
        if (reinterpret_cast<size_t>(hook_id) > 100 &&
            reinterpret_cast<size_t>(hook_id) <=
              (100 + g_argsOffsets.size())) {
          // c->Print("Operator Leave: %i", reinterpret_cast<int>(hook_id));
          if (g_eventArgsPointer != nullptr && g_eventName != nullptr) {
            auto argsArray =
              static_cast<RE::BSScrapArray<RE::BSScript::Variable>*>(
                g_eventArgsPointer);
            if (argsArray->size() > 0) {
              c->Print("Operator ARGS Leave: %i, theadID: %d, eventName: %s",
                       argsArray->size(), GetCurrentThreadId(), g_eventName);
            }
            std::vector<CallNative::AnySafe> out{};
            out.reserve(argsArray->size());
            for (const auto& r : *argsArray) {
              auto rSafe = CallNative::VariableToAnySafe(r, std::nullopt);
              out.push_back(rSafe);
            }
            EventsApi::SendPapyrusEventEnter(
              g_eventSelfId, static_cast<std::string>(g_eventName), out);
            g_eventArgsPointer = nullptr;
            g_eventName = nullptr;
            g_eventSelfId = 0;
            EventsApi::SendPapyrusEventLeave();
          } else if (g_eventName != nullptr) {
              // c->Print("Event without args: %s", g_eventName);
              std::vector<CallNative::AnySafe> out{};
              out.reserve(0);
              EventsApi::SendPapyrusEventEnter(
                g_eventSelfId, static_cast<std::string>(g_eventName), out);
              g_eventName = nullptr;
              g_eventSelfId = 0;
              EventsApi::SendPapyrusEventLeave();
          }
        } else {
          if (g_eventName != nullptr) {
              c->Print("Event without offset hookid: %i, name:  %s",
                       (int)reinterpret_cast<size_t>(hook_id), g_eventName);
          }
        }
      }
    }
  }
}

static void example_listener_class_init(ExampleListenerClass* klass)
{
  (void)EXAMPLE_IS_LISTENER;
#ifndef _MSC_VER
  (void)glib_autoptr_cleanup_ExampleListener;
#endif
}

static void example_listener_iface_init(gpointer g_iface, gpointer iface_data)
{
  auto iface = (GumInvocationListenerInterface*)g_iface;

  iface->on_enter = example_listener_on_enter;
  iface->on_leave = example_listener_on_leave;
}

static void example_listener_init(ExampleListener* self)
{
}
