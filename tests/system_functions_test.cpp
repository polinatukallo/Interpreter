#include <lib/interpreter.h>
#include <gtest/gtest.h>
#include <sstream>

// Test for print statement (no newline)
TEST(SystemFunctionsTestSuite, PrintFunction) {
    std::string code = R"(
        print(42)
    )";
    std::string expected = "42";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test for println function (with newline)
TEST(SystemFunctionsTestSuite, PrintLnFunction) {
    std::string code = R"(
        println(42)
    )";
    std::string expected = "42\n";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test for read function (no additional input: empty string)
TEST(SystemFunctionsTestSuite, ReadFunction) {
    std::string code = R"(
        s = read()
        print(s)
    )";
    std::string expected = "";

    std::istringstream input(code);
    std::ostringstream output;

    bool success = interpret(input, output);
    if (!success) {
        std::cerr << "[ReadFunction DEBUG] Interpreter failed. Error: " << output.str() << std::endl;
    }
    ASSERT_TRUE(success);
    ASSERT_EQ(output.str(), expected);
}

// Test for stacktrace function (global context: empty list)
TEST(SystemFunctionsTestSuite, StackTraceFunction) {
    std::string code = R"(
        print(stacktrace())
    )";
    std::string expected = "[]";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}
