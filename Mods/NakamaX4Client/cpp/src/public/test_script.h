#pragma once
#include "x4_script_base.h"

// Simple test script class for demonstrating auto-generated Lua exports
class TestScript : public X4ScriptSingleton<TestScript>
{
public:
    TestScript() : X4ScriptSingleton("TestScript") {}

    // LUA_EXPORT
    int SumAB(int a, int b) {
        return a + b;
    }

    // LUA_EXPORT
    std::string ConcatStrings(const std::string& str1, const std::string& str2) {
        return str1 + str2;
    }

    // LUA_EXPORT
    bool IsEven(int number) {
        return number % 2 == 0;
    }

    // LUA_EXPORT
    double Multiply(double x, double y) {
        return x * y;
    }

    // LUA_EXPORT
    void Shutdown() override {
        // Test script shutdown
    }

    // LUA_EXPORT
    void Update(float deltaTime) override {
        // Test script update
    }

private:
};