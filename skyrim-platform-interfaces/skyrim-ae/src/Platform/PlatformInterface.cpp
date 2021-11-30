/**
 * this should connect platform to interface
 *
 * Browser API wants RE::UIMessageQueue::AddMessage
 * and RE::UI::IsOpen(cursorMenu)
 *
 * CallNative is hard gotta research
 *
 * Camera API exposes WorldPointToScreenPoint
 * with SKSE Requirements NiCamera, Player:GetSingleton() etc
 *
 * Console API wants to PrintConsole and Run commands?
 * Log to file should be handled at Platform?
 *
 * Dev API wants MenuEventHandler and MenuControls to disable a key?
 *
 * DumpFunctions wants info about native types, should go to interface side?
 *
 * Event API wants to subscribe to events
 *
 * FunctionInfo wants RE::BSScript::TypeInfo::RawType
 * and RE::BSTSmartPointer<RE::BSScript::IFunction>
 *
 * Inventory API need player state and inventory state data (duh)
 *
 * LoadGame want info about save files and various other data like time,
 * weather, coords etc
 *
 * Skyrim Platform needs an endpoint to add tasks
 *
 * VM lives on interface's side but gotta expose some API?
 */
