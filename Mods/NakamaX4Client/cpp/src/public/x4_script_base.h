#pragma once
#include <string>
#include <memory>
#include <cstdarg>
#include <vector>
#include <functional>
#include <unordered_map>
#include "log_to_x4.h"

extern "C"
{
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
}

// Abstract base class for X4 C++ scripts
// Provides common functionality like logging, initialization, and Lua integration
class X4ScriptBase
{
protected:
    std::string m_scriptName;
    bool m_initialized;

public:
    X4ScriptBase(const std::string &scriptName)
        : m_scriptName(scriptName), m_initialized(false) {}

    virtual ~X4ScriptBase() = default;

    // Pure virtual methods that derived classes must implement
    virtual bool Initialize() { return true; } // Default implementation
    virtual void Shutdown() = 0;
    virtual void Update(float deltaTime)
    {
        // Call all registered update callbacks
        for (const auto &callback : m_registeredCallbacks)
        {
            try
            {
                callback.second(deltaTime);
            }
            catch (const std::exception &e)
            {
                LogError("Exception in update callback: %s", e.what());
            }
        }
    }

    // Common functionality
    const std::string &GetScriptName() const { return m_scriptName; }
    bool IsInitialized() const { return m_initialized; }

    // Register a C++ callback (called every Update)
    virtual int RegisterUpdateCallback(std::function<void(float)> callback)
    {
        int callbackId = m_nextCallbackId++;
        m_registeredCallbacks[callbackId] = callback;
        return callbackId;
    }

    virtual void UnregisterUpdateCallback(int callbackId)
    {
        m_registeredCallbacks.erase(callbackId);
    }

protected:
    void SetInitialized(bool initialized) { m_initialized = initialized; }

    // Logging helpers
    void LogInfo(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        LogToX4::Log("[%s] %s", m_scriptName.c_str(), FormatMessage(fmt, args).c_str());
        va_end(args);
    }

    void LogError(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        LogToX4::Log("[%s] ERROR: %s", m_scriptName.c_str(), FormatMessage(fmt, args).c_str());
        va_end(args);
    }

    void LogWarning(const char *fmt, ...)
    {
        va_list args;
        va_start(args, fmt);
        LogToX4::Log("[%s] WARNING: %s", m_scriptName.c_str(), FormatMessage(fmt, args).c_str());
        va_end(args);
    }

private:
    std::string FormatMessage(const char *fmt, va_list args)
    {
        char buffer[4096];
        vsnprintf_s(buffer, sizeof(buffer), _TRUNCATE, fmt, args);
        return std::string(buffer);
    }

    std::vector<std::function<void()>> m_updateCallbacks;
    int m_nextCallbackId = 0;
    std::unordered_map<int, std::function<void(float)>> m_registeredCallbacks;
};

// Template class for singleton script instances
template <typename T>
class X4ScriptSingleton : public X4ScriptBase
{
protected:
    static std::unique_ptr<T> s_instance;

public:
    static T *GetInstance()
    {
        if (!s_instance)
        {
            s_instance = std::make_unique<T>();
        }
        return s_instance.get();
    }

    static void DestroyInstance()
    {
        s_instance.reset();
    }

protected:
    X4ScriptSingleton(const std::string &scriptName) : X4ScriptBase(scriptName) {}
};

template <typename T>
std::unique_ptr<T> X4ScriptSingleton<T>::s_instance = nullptr;