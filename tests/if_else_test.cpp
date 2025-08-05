#include <lib/interpreter.h>
#include <gtest/gtest.h>
#include <sstream>

// Тест для условных операторов (if, else if, else)
TEST(ControlFlowTestSuite, ConditionalStatements1) {
    std::string code = R"(
        x = 10
        if x > 15 then
            print("Greater")
        else if x > 5 then
            print("Medium")
        else
            print("Small")
        end if
        y = 0
        if x == 10 and y == 0 then
            print("BothTrue")
        else
            print("False")
        end if
    )";

    std::string expected = "MediumBothTrue";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

// Тест для цикла while с break и continue
TEST(ControlFlowTestSuite, WhileLoopWithBreakAndContinue1) {
    std::string code = R"(
        i = 0
        while i < 5
            i = i + 1
            if i == 2 then
                continue
            end if
            if i == 4 then
                break
            end if
            print(i)
        end while
    )";

    std::string expected = "13";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

TEST(ControlFlowTestSuite, BreakInForLoop) {
    std::string code = R"(
        sum = 0
        for i in [1, 2, 3, 4, 5]
            if i == 3 then
                break
            end if
            sum = sum + i
        end for
        print(sum)
    )";

    std::string expected = "3";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

TEST(ControlFlowTestSuite, ContinueInForLoop) {
    std::string code = R"(
        sum = 0
        for i in [1, 2, 3, 4, 5]
            if i % 2 == 0 then
                continue
            end if
            sum = sum + i
        end for
        print(sum)
    )";

    std::string expected = "9";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}

TEST(ControlFlowTestSuite, SimpleLoop) {
    std::string code = R"(
        a = 0
        b = 1
        n = 10
        for i in range(n - 2)
            c = a + b + i
            print(c)
        end for
    )";

    std::string expected = "12345678";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_TRUE(interpret(input, output));
    ASSERT_EQ(output.str(), expected);
}