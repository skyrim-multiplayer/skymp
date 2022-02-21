#pragma once

namespace Frida {
typedef struct _InvocationListener InvocationListener;

struct _InvocationListener
{
  GObject parent;
  guint num_calls;
};

enum HookID
{
  HOOK_SEND_ANIMATION_EVENT,
  DRAW_SHEATHE_WEAPON_ACTOR,
  DRAW_SHEATHE_WEAPON_PC,
  QUEUE_NINODE_UPDATE,
  APPLY_MASKS_TO_RENDER_TARGET,
  RENDER_CURSOR_MENU,
  SEND_EVENT,
  SEND_EVENT_ALL,
  CONSOLE_VPRINT
};

struct Hook
{
  Hook(std::function<void(GumInvocationContext*)> enter,
       std::function<void(GumInvocationContext*)> leave)
    : onEnter(enter)
    , onLeave(leave)
  {
  }
  std::function<void(GumInvocationContext*)> onEnter;
  std::function<void(GumInvocationContext*)> onLeave;
};

class HookHandler
{
public:
  static HookHandler* GetSingleton()
  {
    static HookHandler singleton;
    return &singleton;
  }

  bool Attach(HookID id, uintptr_t address);
  void Install(HookID id, uintptr_t address, std::shared_ptr<Hook> hook);
  auto GetHooks() { return &hooks; }
  std::function<void(GumInvocationContext*)> GetHookEnterFunction(HookID id);
  std::function<void(GumInvocationContext*)> GetHookLeaveFunction(HookID id);

private:
  HookHandler();

  // with current impl this detaches all hooks
  void Detach()
  {
    gum_interceptor_begin_transaction(_interceptor);
    gum_interceptor_detach(_interceptor, _listener);
    gum_interceptor_end_transaction(_interceptor);
  }

  robin_hood::unordered_map<HookID, std::shared_ptr<Hook>> hooks;

  GumInterceptor* _interceptor;
  GumInvocationListener* _listener;
};
}
