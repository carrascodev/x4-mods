#include <catch2/catch_test_macros.hpp>
#include "x4_script_base.h"

TEST_CASE("X4ScriptBase basic functionality", "[commons]") {
    // Test basic instantiation
    class TestScript : public X4ScriptBase {
    public:
        TestScript() : X4ScriptBase("TestScript") {}
        void Shutdown() override {}
    };

    TestScript script;
    REQUIRE(script.GetScriptName() == "TestScript");
    REQUIRE(!script.IsInitialized());
}