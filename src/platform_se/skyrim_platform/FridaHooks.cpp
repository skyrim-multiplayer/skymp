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
  RENDER_MAIN_MENU
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
  w.Attach(listener, 5367792, RENDER_MAIN_MENU);
}

thread_local uint32_t g_queueNiNodeActorId = 0;
thread_local void* g_prevMainMenuView = nullptr;

bool g_allowHideMainMenu = true;

static void example_listener_on_enter(GumInvocationListener* listener,
                                      GumInvocationContext* ic)
{
  ExampleListener* self = EXAMPLE_LISTENER(listener);
  auto hook_id = gum_invocation_context_get_listener_function_data(ic);

  auto _ic = (_GumInvocationContext*)ic;

  switch ((size_t)hook_id) {
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
    case RENDER_MAIN_MENU: {
      static auto fsMainMenu = new BSFixedString("Cursor Menu");
      auto mainMenu = FridaHooksUtils::GetMenuByName(fsMainMenu);
      auto this_ = (int64_t*)_ic->cpu_context->rcx;
      if (g_allowHideMainMenu) {

        if (this_)
          if (mainMenu == this_) {
            auto viewPtr = reinterpret_cast<void**>(((uint8_t*)this_) + 0x10);
            g_prevMainMenuView = *viewPtr;
            *viewPtr = nullptr;
            // FridaHooksUtils::ResetView(this_);
          }
        /* FridaHooksUtils::GetMenuByName() auto menu =
           (uint8_t*)_ic->cpu_context->rcx;
         *reinterpret_cast<void**>(menu + 0x10) = nullptr;*/
      }
      break;
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
    case RENDER_MAIN_MENU: {
      auto _ic = (_GumInvocationContext*)ic;

      static auto fsMainMenu = new BSFixedString("Cursor Menu");
      auto mainMenu = FridaHooksUtils::GetMenuByName(fsMainMenu);
      auto this_ = (int64_t*)_ic->cpu_context->rcx;
      auto viewPtr = reinterpret_cast<void**>(((uint8_t*)this_) + 0x10);
      bool renderHookInProgress = g_prevMainMenuView != nullptr;
      if (renderHookInProgress)
        if (this_)
          if (mainMenu == this_) {
            *viewPtr = g_prevMainMenuView;
            g_prevMainMenuView = nullptr;
          }
      break;
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
