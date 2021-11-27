/* these functions should be a part of Interface API */
class TaskHandler
{
public:
  static TaskHandler& getInstance()
  {
    static TaskHandler singleton;
    return singleton;
  }

  AddTask(std::function<void> func)
  {
    SKSE::GetTaskInterface()->AddTask(func);
  }

  AddUITask(std::function<void> func)
  {
    SKSE::GetTaskInterface()->AddUITask(func);
  }

private:
  EventHandler() = default;

  ~EventHandler() = default;
};
