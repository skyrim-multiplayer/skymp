#include "FridaHookHandler.h"

namespace Frida {
/**
 * Frida initialization block start
 */
static void invocation_listener_iface_init(gpointer g_iface,
                                           gpointer iface_data);

#define INVOCATION_TYPE_LISTENER (invocation_listener_get_type())
G_DECLARE_FINAL_TYPE(InvocationListener, invocation_listener, INVOCATION,
                     LISTENER, GObject)
G_DEFINE_TYPE_EXTENDED(InvocationListener, invocation_listener, G_TYPE_OBJECT,
                       0,
                       G_IMPLEMENT_INTERFACE(GUM_TYPE_INVOCATION_LISTENER,
                                             invocation_listener_iface_init))

static void invocation_listener_on_enter(GumInvocationListener* listener,
                                         GumInvocationContext* ic)
{
  InvocationListener* self = INVOCATION_LISTENER(listener);
  auto id = (size_t)gum_invocation_context_get_listener_function_data(ic);

  auto func =
    HookHandler::GetSingleton()->GetHookEnterFunction(static_cast<HookID>(id));

  if (func)
    try {
      func(ic);
    } catch (const std::exception& e) {
      logger::critical("Error executing hook enter function: id {} error {}",
                       id, e.what());
    }
}

static void invocation_listener_on_leave(GumInvocationListener* listener,
                                         GumInvocationContext* ic)
{
  InvocationListener* self = INVOCATION_LISTENER(listener);
  auto id = (size_t)gum_invocation_context_get_listener_function_data(ic);

  auto func =
    HookHandler::GetSingleton()->GetHookLeaveFunction(static_cast<HookID>(id));

  if (func)
    try {
      func(ic);
    } catch (const std::exception& e) {
      logger::critical("Error executing hook leave function: id {} error {}",
                       id, e.what());
    }
}

static void invocation_listener_class_init(InvocationListenerClass* klass)
{
  (void)INVOCATION_IS_LISTENER;
}

static void invocation_listener_iface_init(gpointer g_iface,
                                           gpointer iface_data)
{
  auto iface = (GumInvocationListenerInterface*)g_iface;

  iface->on_enter = invocation_listener_on_enter;
  iface->on_leave = invocation_listener_on_leave;
}

static void invocation_listener_init(InvocationListener* self)
{
}

/**
 * Frida initialization block end
 */

HookHandler::HookHandler()
  : hooks()
{
  gum_init_embedded();
  _interceptor = gum_interceptor_obtain();
  _listener =
    (GumInvocationListener*)g_object_new(INVOCATION_TYPE_LISTENER, NULL);
}

bool HookHandler::Attach(HookID id, uintptr_t address)
{
  auto status = gum_interceptor_attach(_interceptor, (void*)address, _listener,
                                       GSIZE_TO_POINTER(id));
  if (status != GUM_ATTACH_OK) {
    logger::critical(
      "Failed to attach hook: address {:X} id {} with status {}.",
      address - Offsets::BaseAddress, id, status);
    return false;
  } else {
    logger::debug("Attached hook: address {:X} id {} with status {}.",
                  address - Offsets::BaseAddress, id, status);
    return true;
  }
}

void HookHandler::Install(HookID id, uintptr_t address,
                          std::shared_ptr<Hook> hook)
{
  if (hooks.contains(id)) {
    logger::critical(
      "Failed to install hook: address {:X} id {} already installed.",
      address - Offsets::BaseAddress, id);
    return;
  }

  gum_interceptor_begin_transaction(_interceptor);
  auto attached = Attach(id, address);
  gum_interceptor_end_transaction(_interceptor);

  if (attached)
    hooks.emplace(id, hook);
}

std::function<void(GumInvocationContext*)> HookHandler::GetHookEnterFunction(
  HookID id)
{
  if (!hooks.contains(id))
    return nullptr;

  return hooks[id]->onEnter;
}

std::function<void(GumInvocationContext*)> HookHandler::GetHookLeaveFunction(
  HookID id)
{
  if (!hooks.contains(id))
    return nullptr;

  return hooks[id]->onLeave;
}
}
