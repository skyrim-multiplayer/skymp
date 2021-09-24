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

    //нужно обновить CommonLibSSE (нету в текущей версии)
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

    //вообще нету в CommonLibSSE
    struct ConsoleOpenHandler : public MenuEventHandler
    {
    public:
        inline static constexpr auto RTTI = RTTI_ConsoleOpenHandler;

        virtual ~ConsoleOpenHandler() = default;  // 00

        // add
        virtual bool CanProcess(InputEvent* a_event) = 0;      // 01
        virtual bool ProcessKinect(KinectEvent* a_event) = 0;  // 02
        virtual bool ProcessButton(ButtonEvent* a_event) = 0;  // 05
    };
    /*
    //вообще нету в CommonLibSSE
    struct QuickSaveLoadHandler : public MenuEventHandler
    {
    public:
        inline static constexpr auto RTTI = RTTI_QuickSaveLoadHandler;

        virtual ~QuickSaveLoadHandler() = default;  // 00

        // add
        virtual bool CanProcess(InputEvent* a_event) = 0;      // 01
        virtual bool ProcessKinect(KinectEvent* a_event) = 0;  // 02
        virtual bool ProcessButton(ButtonEvent* a_event) = 0;  // 05
    };*/
}

//не работает с ключом типа 'char*'
std::map<std::string, char*> keyToMenuName {
    {"Console", "Console"},
    {"Tween Menu", "TweenMenu"},
    {"Favorites", "FavoritesMenu"}
};

bool onMenuOpenClose(const char * keyName) {
    if(keyToMenuName.find(keyName) == keyToMenuName.end())
        return false;

    char* menuName = keyToMenuName[keyName];

    auto lg = RE::ConsoleLog::GetSingleton();
    auto mc = RE::MenuControls::GetSingleton();
    auto ui = RE::UI::GetSingleton();

    if (!lg || !mc || !ui)
        return false;
    

    //lg->Print("onMenuOpenClose '%s', '%s'", keyName, menuName);
    
    g_taskQueue.AddTask([=] {
        auto obj = JsValue::Object();

        obj.SetProperty("name", JsValue::String(menuName));
        obj.SetProperty("type", JsValue::String( ui->IsMenuOpen(menuName) ? "close" : "open"));

        EventsApi::SendEvent("menuOpenClose", { JsValue::Undefined(), obj });
    });

    return true;
}

template <typename T>
struct MyMenuHandler : public RE::FavoritesHandler
{
public:
    MyMenuHandler(const char* keyName_, T* originalHandler_) : originalHandler(originalHandler_), keyName(keyName_){
    }

    ~MyMenuHandler() {
    };

    bool CanProcess(RE::InputEvent* e) override {
        if (e->eventType == RE::INPUT_EVENT_TYPE::kButton) {
            const RE::ButtonEvent* btn = static_cast<const RE::ButtonEvent*>(e);

            auto ui = RE::UI::GetSingleton();

            char* menuName = keyToMenuName[keyName];

            if (ui->IsMenuOpen(menuName) && btn->IsDown())
            {
                if (strcmp(e->QUserEvent().c_str(), keyName) == 0 || strcmp(e->QUserEvent().c_str(), "Cancel") == 0)
                {
                    onMenuOpenClose(keyName);
                }
            }

            /*if (strcmp(keyName, "QuickSaveLoad")) {
                auto lg = RE::ConsoleLog::GetSingleton();
                lg->Print("-------------------MyMenuHandler::CanProcess '%s'", e->QUserEvent());
            }*/
        }

        return originalHandler->CanProcess(e);
    };

    bool ProcessKinect(RE::KinectEvent* e) override {
        return originalHandler->ProcessKinect(e);
    };

    bool ProcessButton(RE::ButtonEvent* e) override {
        return originalHandler->ProcessButton(e);

    };

private:
    const char* keyName;
    T* originalHandler;
};


struct PlaceholderMenuHandler : public RE::MenuEventHandler
{
    PlaceholderMenuHandler() {}

    ~PlaceholderMenuHandler() {};

    bool CanProcess(RE::InputEvent* e) override { return false; };

    bool ProcessKinect(RE::KinectEvent* e) override { return false; };

    bool ProcessButton(RE::ButtonEvent* e) override { return false; };
};

template <typename T>
struct MyMenuOpenEventHandler : public RE::MenuEventHandler
{
public:
    MyMenuOpenEventHandler(T* originalHandler_) {
        originalHandler = originalHandler_;
    }
    ~MyMenuOpenEventHandler() {}

    bool CanProcess(RE::InputEvent* e) override {
        if (e->eventType == RE::INPUT_EVENT_TYPE::kButton) {
            const RE::ButtonEvent* btn = static_cast<const RE::ButtonEvent*>(e);
            if (btn->IsDown()) {
                //auto lg = RE::ConsoleLog::GetSingleton();
                //lg->Print("MyMenuOpenEventHandler::CanProcess '%s'", e->QUserEvent());
                onMenuOpenClose(e->QUserEvent().c_str());
            }
        }

        return originalHandler->CanProcess(e);
    }

    bool ProcessKinect(RE::KinectEvent* e) override {
        //auto lg = RE::ConsoleLog::GetSingleton();
        //lg->Print("MyMenuOpenEventHandler::ProcessKinect");

        return originalHandler->ProcessKinect(e);
    }

    bool ProcessButton(RE::ButtonEvent* e) override {
        //auto lg = RE::ConsoleLog::GetSingleton();
        //lg->Print("MyMenuOpenEventHandler::ProcessButton '%s':'%d'", e->QUserEvent(), e->value);

        return originalHandler->ProcessButton(e);
    }

    // members
    bool   unk10;  // 10
    UInt8  unk11;  // 11
    UInt16 unk12;  // 12
    UInt32 unk14;  // 14

private:
    T* originalHandler;
};

//переделать disableOriginalConsole() -> disableUi("Console")
void disableOriginalConsole()
{
    auto mc = RE::MenuControls::GetSingleton();
    
    if (!mc)
        return;

    PlaceholderMenuHandler* consoleOpenHandler = new PlaceholderMenuHandler;
    mc->RemoveHandler(mc->consoleOpenHandler.get());
    mc->AddHandler(consoleOpenHandler);
    mc->consoleOpenHandler = RE::BSTSmartPointer<RE::ConsoleOpenHandler>((RE::ConsoleOpenHandler*)consoleOpenHandler);
}

namespace UiApi
{
    void Register(JsValue& exports)
    {
        auto lg = RE::ConsoleLog::GetSingleton();
        auto mc = RE::MenuControls::GetSingleton();
        auto ui = RE::UI::GetSingleton();

        if (!lg || !mc || !ui)
            return;

        //menuOpenHandler
        MyMenuOpenEventHandler<RE::MenuEventHandler>* menuOpenHandler = new MyMenuOpenEventHandler<RE::MenuEventHandler>(mc->menuOpenHandler.get());
        mc->RemoveHandler(mc->menuOpenHandler.get());
        mc->AddHandler(menuOpenHandler);
        mc->menuOpenHandler = RE::BSTSmartPointer<RE::MenuOpenHandler>((RE::MenuOpenHandler*)menuOpenHandler);

        //favoritesHandler
        MyMenuHandler<RE::FavoritesHandler>* favoritesHandler = new MyMenuHandler<RE::FavoritesHandler>("Favorites", mc->favoritesHandler.get());
        mc->RemoveHandler(mc->favoritesHandler.get());
        mc->AddHandler(favoritesHandler);
        mc->favoritesHandler = RE::BSTSmartPointer<RE::FavoritesHandler>((RE::FavoritesHandler*)favoritesHandler);
        /*
        //quickSaveLoadHandler
        MyMenuHandler<RE::QuickSaveLoadHandler>* quickSaveLoadHandler = new MyMenuHandler<RE::QuickSaveLoadHandler>("QuickSaveLoad", mc->quickSaveLoadHandler.get());
        mc->RemoveHandler(mc->quickSaveLoadHandler.get());
        mc->AddHandler(quickSaveLoadHandler);
        mc->quickSaveLoadHandler = RE::BSTSmartPointer<RE::QuickSaveLoadHandler>((RE::QuickSaveLoadHandler*)quickSaveLoadHandler);
        */
        auto uiObj = JsValue::Object();
        uiObj.SetProperty(
            "disableOriginalConsole",
            JsValue::Function([=](const JsFunctionArguments& args) -> JsValue {
                disableOriginalConsole();
                return JsValue::Undefined();
            }));
        exports.SetProperty("ui", uiObj);
    }
}