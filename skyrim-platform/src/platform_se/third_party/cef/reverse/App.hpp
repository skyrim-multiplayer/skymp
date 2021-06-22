#pragma once

namespace CEFUtils
{
    struct SKSEPluginBase
    {
        SKSEPluginBase() noexcept;
        virtual ~SKSEPluginBase();

        // Retrieve the program's main address, if null is returned the main function will not be hooked
        [[nodiscard]] virtual void* GetMainAddress() const = 0;

        // Called right before the program's main function is called
        virtual bool BeginMain() = 0;

        // Called after the program's main function exits
        virtual bool EndMain() = 0;

        // Called when the dll is attached, before anything else
        virtual bool Attach() = 0;

        // Called when the dll is detached
        virtual bool Detach() = 0;

        // Must be called once per frame
        virtual void Update() = 0;

        void Start() noexcept;

        // Functions that can be called by class users
        [[nodiscard]] bool IsReady() const noexcept;


        static SKSEPluginBase& GetInstance() noexcept;

    private:

        bool m_ready;
    };
}
