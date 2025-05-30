#include <lib/interpreter.h>
#include <gtest/gtest.h>
#include <sstream>

// Test range(start, end) for lists
TEST(ListFunctionsTestSuite, RangeTwoArgs) {
    std::string code = R"(
        print(range(1, 5))
    )";
    std::string expected = "[1, 2, 3, 4]";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test len(list)
TEST(ListFunctionsTestSuite, LenList) {
    std::string code = R"(
        print(len([10, 20, 30]))
    )";
    std::string expected = "3";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test push(list, x)
TEST(ListFunctionsTestSuite, Push) {
    std::string code = R"(
        l = [1, 2]
        push(l, 3)
        print(l)
    )";
    std::string expected = "[1, 2, 3]";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test pop(list)
TEST(ListFunctionsTestSuite, Pop) {
    std::string code = R"(
        l = [1, 2, 3]
        print(pop(l))
        print(l)
    )";
    std::string expected = "3[1, 2]";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test insert(list, index, x)
TEST(ListFunctionsTestSuite, Insert) {
    std::string code = R"(
        l = [1, 3]
        insert(l, 1, 2)
        print(l)
    )";
    std::string expected = "[1, 2, 3]";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test remove(list, index)
TEST(ListFunctionsTestSuite, Remove) {
    std::string code = R"(
        l = [1, 2, 3]
        print(remove(l, 1))
        print(l)
    )";
    std::string expected = "2[1, 3]";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test sort(list) for numbers
TEST(ListFunctionsTestSuite, SortNumeric) {
    std::string code = R"(
        l = [3, 1, 2]
        sort(l)
        print(l)
    )";
    std::string expected = "[1, 2, 3]";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Test sort(list) for strings
TEST(ListFunctionsTestSuite, SortString) {
    std::string code = R"(
        l = ["b", "a", "c"]
        sort(l)
        print(l)
    )";
    std::string expected = "[\"a\", \"b\", \"c\"]";

    std::istringstream input(code);
    std::ostringstream output;
    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}
