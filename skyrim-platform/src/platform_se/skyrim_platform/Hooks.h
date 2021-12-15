namespace Hooks {
  void Hooks::InstallDeviceConnectHook()
{
	REL::Relocation<std::uintptr_t> hook{ Offset::InputManager::ProcessEvent, 0x7E };
	REL::safe_fill(hook.address(), REL::NOP, 0x6);
}
}