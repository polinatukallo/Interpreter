#include <lib/interpreter.h>
#include <gtest/gtest.h>
#include <sstream>

// Test len(s)
TEST(StringFunctionsTestSuite, Len) {
    std::string code = R"(
        print(len("hello"))
    )";
    std::string expected = "5";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test lower(s)
TEST(StringFunctionsTestSuite, Lower) {
    std::string code = R"(
        print(lower("HeLLo"))
    )";
    std::string expected = "hello";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test upper(s)
TEST(StringFunctionsTestSuite, Upper) {
    std::string code = R"(
        print(upper("HeLLo"))
    )";
    std::string expected = "HELLO";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test split(s, delim)
TEST(StringFunctionsTestSuite, Split) {
    std::string code = R"(
        print(split("a,b,c", ","))
    )";
    std::string expected = "[\"a\", \"b\", \"c\"]";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test join(list, delim)
TEST(StringFunctionsTestSuite, Join) {
    std::string code = R"(
        print(join(["a","b","c"], ","))
    )";
    std::string expected = "a,b,c";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test replace(s, old, new)
TEST(StringFunctionsTestSuite, Replace) {
    std::string code = R"(
        print(replace("abracadabra", "a", "o"))
    )";
    std::string expected = "obrocodobro";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}
