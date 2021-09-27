#include "UiApi.h"
#include "EventsApi.h"
#include "GameEventSinks.h"

#include <RE/MenuControls.h>
#include <RE/UI.h>
#include <RE/Console.h>
#include <RE/ConsoleLog.h>
#include <RE/ButtonEvent.h>
#include <RE/MenuEventHandler.h>
#include <RE/MenuOpenHandler.h>

#include <map>

extern TaskQueue g_taskQueue;

namespace RE {
    //need to update CommonLibSSE (missing in the current version)
    //https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/blob/238e7815d5e7e0c4a26491d15c45197390391d75/include/RE/F/FavoritesHandler.h
    struct FavoritesHandler : public MenuEventHandler
    {
    public:
        inline static constexpr auto RTTI = RTTI_FavoritesHandler;

        virtual ~FavoritesHandler() = default;  // 00

        // add
        virtual bool CanProcess(InputEvent* a_event) = 0;      // 01
        virtual bool ProcessKinect(KinectEvent* a_event) = 0;  // 02
        virtual bool ProcessButton(ButtonEvent* a_event) = 0;  // 05
    };
    static_assert(sizeof(FavoritesHandler) == 0x10);

    //need to update CommonLibSSE (missing in the current version)
    //https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/blob/01374c521e6ab485e5aca4de83a2d2fdc4c9c3c1/include/RE/M/MenuOpenCloseEvent.h
    class MenuOpenCloseEvent
    {
    public:
        // members
        BSFixedString menuName;  // 00
        bool          opening;   // 08
        std::uint8_t  pad09;     // 09
        std::uint16_t pad0A;     // 0A
        std::uint32_t pad0C;     // 0C
    };
    static_assert(sizeof(MenuOpenCloseEvent) == 0x10);
    //>>>

    //<<<need to update CommonLibSSE (missing in the current version)
    //https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/blob/4e38a7eeca13aa0655b057e584a3e72f5497595b/include/RE/I/IUIMessageData.h
    class IUIMessageData
    {
    public:
        inline static constexpr auto RTTI = RTTI_IUIMessageData;

        virtual ~IUIMessageData() = default;  // 00

        // members
        std::uint16_t unk08;  // 08
        std::uint16_t pad0A;  // 0A
        std::uint32_t pad0C;  // 0C
    };
    static_assert(sizeof(IUIMessageData) == 0x10);
    //>>>

    //need to update CommonLibSSE (missing in the current version)
    //https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/blob/01374c521e6ab485e5aca4de83a2d2fdc4c9c3c1/include/RE/U/UIMessage.h
    enum class UI_MESSAGE_TYPE : std::uint32_t
    {
        kUpdate = 0,
        kShow = 1,
        kReshow = 2,
        kHide = 3,
        kForceHide = 4,

        kScaleformEvent = 6,   // BSUIScaleformData
        kUserEvent = 7,        // BSUIMessageData
        kInventoryUpdate = 8,  // InventoryUpdateData
        kUserProfileChange = 9,
        kMUStatusChange = 10,
        kResumeCaching = 11,
        kUpdateController = 12,
        kChatterEvent = 13
    };

    class UIMessage
    {
    public:
        BSFixedString   menu;
        UI_MESSAGE_TYPE type;
        std::uint32_t   pad0C;
        IUIMessageData* data;
        bool            isPooled;
        std::uint8_t    pad19;
        std::uint16_t   pad1A;
        std::uint32_t   pad1C;
    };
    static_assert(sizeof(UIMessage) == 0x20);
    //>>>

    //need to update CommonLibSSE (missing in the current version)
    //https://github.com/Ryan-rsm-McKenzie/CommonLibSSE/blob/238e7815d5e7e0c4a26491d15c45197390391d75/include/RE/C/ConsoleData.h
    class NiBinaryStream;

    class ConsoleData : public IUIMessageData
    {
    public:
        inline static constexpr auto RTTI = RTTI_ConsoleData;

        enum class DataType: std::uint32_t
        {
        };

        ~ConsoleData() override;  // 00

        // members
        BSString*       str;
        ObjectRefHandle pickRef;
        std::uint32_t   pad1C;
        NiBinaryStream* file;
        DataType        type;
        std::uint32_t   pad2C;
    };
    static_assert(sizeof(ConsoleData) == 0x30);
    //>>>
}

void onMenuOpenClose(const char * menuName, bool opening) {
    g_taskQueue.AddTask([=] {
        auto obj = JsValue::Object();

        obj.SetProperty("name", JsValue::String(menuName));
        obj.SetProperty("type", JsValue::String( opening ? "open" : "close"));

        EventsApi::SendEvent("menuOpenClose", { JsValue::Undefined(), obj });
    });
}

class MyMenu :  public RE::IMenu,
                public RE::MenuEventHandler
{
public:
    using IMenu::operator new;
    using IMenu::operator delete;

    MyMenu(char* cancelKeyName_, char* menuName_, RE::IMenu* originalMenu_) : cancelKeyName(cancelKeyName_), menuName(menuName_), originalMenu(originalMenu_){}

    ~MyMenu() {}

    void Accept(CallbackProcessor * a_processor) override {
        auto lg = RE::ConsoleLog::GetSingleton();
        lg->Print("MyMenu::Accept");
    };

    void PostCreate() override {};
    void Unk_03(void) override {};

    RE::UI_MESSAGE_RESULTS ProcessMessage(RE::UIMessage & msg) override {
        if(strcmp(msg.menu.c_str(), "FavoritesMenu") != 0)
            return RE::UI_MESSAGE_RESULTS::kIgnore;


        auto lg = RE::ConsoleLog::GetSingleton();
        auto ui = RE::UI::GetSingleton();

        switch (msg.type)
        {
        case RE::UI_MESSAGE_TYPE::kUpdate:
            lg->Print("MyMenu::ProcessMessage::kUpdate");
            break;
        case RE::UI_MESSAGE_TYPE::kShow:
            lg->Print("MyMenu::ProcessMessage::kShow '%u' '%u'", msg.type, msg.isPooled);
            //return RE::UI_MESSAGE_RESULTS::kHandled;
            break;
        case RE::UI_MESSAGE_TYPE::kReshow:
            lg->Print("MyMenu::ProcessMessage::kReshow");
            break;
        case RE::UI_MESSAGE_TYPE::kHide:
            lg->Print("MyMenu::ProcessMessage::kHide");
            //return RE::UI_MESSAGE_RESULTS::kHandled;
            break;
        case RE::UI_MESSAGE_TYPE::kForceHide:
            lg->Print("MyMenu::ProcessMessage::kForceHide");
            //return RE::UI_MESSAGE_RESULTS::kHandled;
            break;
        case RE::UI_MESSAGE_TYPE::kScaleformEvent:
            lg->Print("MyMenu::ProcessMessage::kScaleformEvent");
            //return RE::UI_MESSAGE_RESULTS::kHandled;
            break;
        case RE::UI_MESSAGE_TYPE::kUserEvent:
            lg->Print("MyMenu::ProcessMessage::kUserEvent");
            //return RE::UI_MESSAGE_RESULTS::kHandled;
            break;
        case RE::UI_MESSAGE_TYPE::kInventoryUpdate:
            lg->Print("MyMenu::ProcessMessage::kInventoryUpdate");
            break;
        case RE::UI_MESSAGE_TYPE::kUserProfileChange:
            lg->Print("MyMenu::ProcessMessage::kUserProfileChange");
            break;
        case RE::UI_MESSAGE_TYPE::kMUStatusChange:
            lg->Print("MyMenu::ProcessMessage::kMUStatusChange");
            break;
        case RE::UI_MESSAGE_TYPE::kResumeCaching:
            lg->Print("MyMenu::ProcessMessage::kResumeCaching");
            break;
        case RE::UI_MESSAGE_TYPE::kUpdateController:
            lg->Print("MyMenu::ProcessMessage::kUpdateController");
            break;
        case RE::UI_MESSAGE_TYPE::kChatterEvent:
            lg->Print("MyMenu::ProcessMessage::kChatterEvent");
            break;
        default:
            break;
        }

        return RE::UI_MESSAGE_RESULTS::kHandled;
    };
    void AdvanceMovie(float a_interval, UInt32 a_currentTime) override {};
    void PostDisplay() override {};
    void PreDisplay() override {};
    void RefreshPlatform() override {};

    bool CanProcess(RE::InputEvent* e) override {
        auto ui = RE::UI::GetSingleton();
        auto mc = RE::MenuControls::GetSingleton();

        if (e->eventType != RE::INPUT_EVENT_TYPE::kButton)
            return false;

        const RE::ButtonEvent* btn = static_cast<const RE::ButtonEvent*>(e);
        if (!btn->IsDown())
            return false;

        if (strcmp(e->QUserEvent().c_str(), cancelKeyName) != 0)
            return false;

        if (OnStack())
            CloseMenu();
        else
            OpenMenu();

        //RequiresUpdate();

        return false;
    }
    bool ProcessKinect(RE::KinectEvent* a_event) override { return false; }
    bool ProcessThumbstick(RE::ThumbstickEvent* a_event) override { return false; }
    bool ProcessMouseMove(RE::MouseMoveEvent* a_event) override { return false; }
    bool ProcessButton(RE::ButtonEvent* a_event) override { return false; }

private:
    char* cancelKeyName;
    char* menuName;
    RE::IMenu* originalMenu;

    RE::GPtr<RE::IMenu> GetMenu()
    {
        return RE::UI::GetSingleton()->GetMenu(menuName);
    }

    void CloseMenu()
    {
        auto lg = RE::ConsoleLog::GetSingleton();
        lg->Print("MyMenu::CloseMenu");
        auto ui = RE::UI::GetSingleton();

        RE::MenuOpenCloseEvent* menu_evt = new RE::MenuOpenCloseEvent;

        menu_evt->menuName = menuName;
        menu_evt->opening = true;
        menu_evt->pad09 = 0;
        menu_evt->pad0A = 0;
        menu_evt->pad0C = 0;

        ui->GetEventSource<RE::MenuOpenCloseEvent>()->SendEvent(menu_evt);

        flags = RE::UI_MENU_FLAGS::kOnStack;

        ui->menuStack.push_back(GetMenu());
    }

    void OpenMenu()
    {
        auto lg = RE::ConsoleLog::GetSingleton();
        lg->Print("MyMenu::OpenMenu");
        auto ui = RE::UI::GetSingleton();
        RE::MenuOpenCloseEvent* menu_evt = new RE::MenuOpenCloseEvent;

        menu_evt->menuName = menuName;
        menu_evt->opening = false;
        menu_evt->pad09 = 0;
        menu_evt->pad0A = 0;
        menu_evt->pad0C = 0;

        ui->GetEventSource<RE::MenuOpenCloseEvent>()->SendEvent(menu_evt);

        flags = RE::UI_MENU_FLAGS::kNone;

        auto it = std::find(ui->menuStack.begin(), ui->menuStack.end(), GetMenu());
        if (it != ui->menuStack.end()) {
            ui->menuStack.erase(it);
        }
    }
};

RE::IMenu* creatorFavoritesMenu()
{
    auto mc = RE::MenuControls::GetSingleton();
    auto ui = RE::UI::GetSingleton();

    MyMenu* menu = new MyMenu("Favorites", "FavoritesMenu", ui->GetMenu("FavoritesMenu").get());

    mc->RemoveHandler(mc->favoritesHandler.get());
    mc->favoritesHandler = RE::BSTSmartPointer<RE::FavoritesHandler>((RE::FavoritesHandler*)(RE::MenuEventHandler*)menu);
    mc->AddHandler(mc->favoritesHandler.get());

    return (RE::IMenu*)menu;
}

void disableMenu(std::string menuName)
{
    RE::IMenu* (*creator)(void);
    
    if (menuName == "FavoritesMenu")
    {
        creator = creatorFavoritesMenu;
    }
    else
    {
        return;
    }

    RE::UI::GetSingleton()->menuMap.insert_or_assign({ menuName.c_str(), { nullptr, creator } });
}

void toggleMenu(std::string menuName, bool opening)
{
    auto lg = RE::ConsoleLog::GetSingleton();
    auto mc = RE::MenuControls::GetSingleton();
    auto ui = RE::UI::GetSingleton();

    if (!lg || !mc || !ui)
        return;

    /*RE::MenuOpenCloseEvent* e = new RE::MenuOpenCloseEvent;

    e->menuName = menuName;
    e->opening = opening;
    e->pad09 = 0;
    e->pad0A = 0;
    e->pad0C = 0;

    ui->GetEventSource<RE::MenuOpenCloseEvent>()->SendEvent(e);
    */
}

class MyEventSink : public RE::BSTEventSink<RE::MenuOpenCloseEvent>
{
public:
    ~MyEventSink() {};
    RE::BSEventNotifyControl ProcessEvent(const RE::MenuOpenCloseEvent* e, RE::BSTEventSource<RE::MenuOpenCloseEvent>* a_eventSource) override {
        onMenuOpenClose(e->menuName.c_str(), e->opening);

        return RE::BSEventNotifyControl::kContinue;
    };
};

namespace UiApi
{
    void Register(JsValue& exports)
    {
        auto lg = RE::ConsoleLog::GetSingleton();
        auto mc = RE::MenuControls::GetSingleton();
        auto ui = RE::UI::GetSingleton();

        if (!lg || !mc || !ui)
            return;

        ui->GetEventSource<RE::MenuOpenCloseEvent>()->AddEventSink(new MyEventSink);

        auto uiObj = JsValue::Object();
        uiObj.SetProperty(
            "disableMenu",
            JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
                disableMenu(args[1].ToString());
                return JsValue::Undefined();
            })
        );
        uiObj.SetProperty(
            "toggleMenu",
            JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
                toggleMenu(args[1].ToString(), static_cast<bool>(args[2]));
                return JsValue::Undefined();
            })
        );
        exports.SetProperty("ui", uiObj);
    }
}