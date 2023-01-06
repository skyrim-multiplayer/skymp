#include "TestUtils.hpp"
#include <catch2/catch_all.hpp>

#include "PapyrusForm.h"

using namespace std::chrono_literals;

TEST_CASE("RegisterForSingleUpdate", "[Papyrus][Form]")
{
  class CustomForm : public MpForm
  {
  public:
    void Update() override { counter++; }

    int counter = 0;
  };

  PartOne p;
  p.worldState.AddForm(std::make_unique<CustomForm>(), 0xff000000);

  auto& form = p.worldState.GetFormAt<CustomForm>(0xff000000);

  PapyrusForm().RegisterForSingleUpdate(form.ToVarValue(),
                                        { VarValue(0.05f) });
  REQUIRE(form.counter == 0);
  p.worldState.Tick();
  REQUIRE(form.counter == 0);

  std::this_thread::sleep_for(50ms);

  // Still nothing. This variable is going to be updated by TickTimers
  REQUIRE(form.counter == 0);

  // TickTimers performs Update of our form
  p.worldState.Tick();
  REQUIRE(form.counter == 1);

  // Update must be really single ...
  p.worldState.Tick();
  REQUIRE(form.counter == 1);

  // ... even if 50ms passed again
  std::this_thread::sleep_for(50ms);
  p.worldState.Tick();
  REQUIRE(form.counter == 1);
}

TEST_CASE("RegisterForSingleUpdate triggers Papyrus OnUpdate event",
          "[Papyrus][Form]")
{
  class CustomForm : public MpForm
  {
  public:
    void SendPapyrusEvent(const char* eventName, const VarValue* arguments,
                          size_t argumentsCount)
    {
      if (!strcmp(eventName, "OnUpdate") && !argumentsCount)
        sent = true;
    }

    bool sent = false;
  };

  PartOne p;
  p.worldState.AddForm(std::make_unique<CustomForm>(), 0xff000000);

  auto& form = p.worldState.GetFormAt<CustomForm>(0xff000000);
  PapyrusForm().RegisterForSingleUpdate(form.ToVarValue(), { VarValue(0.f) });

  p.worldState.Tick();
  REQUIRE(form.sent);
}

TEST_CASE("RegisterForSingleUpdate order", "[Papyrus][Form]")
{
  class OrderCountingForm : public MpForm
  {
  public:
    void Update() override { Order().push_back(GetFormId()); }

    std::vector<uint32_t>& Order()
    {
      static std::vector<uint32_t> order;
      return order;
    }
  };

  PartOne p;
  for (auto id : { 0xff000000, 0xff000001, 0xff000002 })
    p.worldState.AddForm(std::make_unique<OrderCountingForm>(), id);

  auto& form0 = p.worldState.GetFormAt<OrderCountingForm>(0xff000000);
  PapyrusForm().RegisterForSingleUpdate(form0.ToVarValue(),
                                        { VarValue(0.010f) });

  auto& form2 = p.worldState.GetFormAt<OrderCountingForm>(0xff000002);
  PapyrusForm().RegisterForSingleUpdate(form2.ToVarValue(),
                                        { VarValue(0.012f) });

  auto& form1 = p.worldState.GetFormAt<OrderCountingForm>(0xff000001);
  PapyrusForm().RegisterForSingleUpdate(form1.ToVarValue(),
                                        { VarValue(0.011f) });

  std::this_thread::sleep_for(12ms * 2);
  p.worldState.Tick();

  REQUIRE(OrderCountingForm().Order() ==
          std::vector<uint32_t>{ 0xff000000, 0xff000001, 0xff000002 });
}
