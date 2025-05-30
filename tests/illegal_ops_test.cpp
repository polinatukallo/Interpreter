#include <string>
#include <vector>
#include <algorithm> // Required for std::equal
#include <sstream>   // Required for std::istringstream and std::ostringstream
// #include <iostream>  // Required for std::cerr (NO LONGER NEEDED)

#include <lib/interpreter.h>
#include <gtest/gtest.h>


std::string kUnreachable = "239";

// Helper function to check if a string ends with a suffix
bool ends_with(const std::string& value, const std::string& suffix) {
    if (suffix.size() > value.size()) return false;
    return std::equal(suffix.rbegin(), suffix.rend(), value.rbegin());
}

TEST(IllegalOperationsSuite, TypeMixing) {
    std::vector<std::string> values = {
        "123",
        "\"string\"", // String literal needs to be escaped for C++ string
        "[1, 2, 3]",
        "function() return 1 end function", // Simplified function for type testing
        "nil",
    };

    for (size_t a = 0; a < values.size(); ++a) {
        for (size_t b = 0; b < values.size(); ++b) {
            std::string code_str;
            code_str += "a = " + values[a] + "\n";
            code_str += "b = " + values[b] + "\n";
            code_str += "c = a + b\n";
            code_str += "print(239) // unreachable\n";

            std::istringstream input(code_str);
            std::ostringstream output;

            bool expected_to_pass = false;
            // Check for valid + operations
            bool a_is_num = (values[a] == "123");
            bool b_is_num = (values[b] == "123");
            bool a_is_str = (values[a] == "\"string\"");
            bool b_is_str = (values[b] == "\"string\"");
            bool a_is_list = (values[a] == "[1, 2, 3]");
            bool b_is_list = (values[b] == "[1, 2, 3]");

            if ((a_is_num && b_is_num) || 
                (a_is_str && b_is_str) ||
                (a_is_list && b_is_list)) {
                expected_to_pass = true;
            }
            
            // Special case: string * number is allowed, but not number * string (handled by interpreter)
            // This test is for `+`, so we don't need to consider `*` here.

            if (expected_to_pass) {
                // std::cerr << "\n[TEST_LOG] Expecting PASS for code:\n" << code_str << "--------------------" << std::endl;
                // bool result = interpret(input, output);
                // std::cerr << "[TEST_LOG] interpret() returned: " << (result ? "true" : "false") << " for expected PASS. Output: " << output.str() << "--------------------\n" << std::endl;
                ASSERT_TRUE(interpret(input, output))
                    << "Code: " << code_str << "\nOutput: " << output.str();
                ASSERT_TRUE(ends_with(output.str(), kUnreachable))
                    << "Code: " << code_str << "\nOutput: " << output.str();
            } else {
                // std::cerr << "\n[TEST_LOG] Expecting FAIL for code:\n" << code_str << "--------------------" << std::endl;
                // bool result = interpret(input, output);
                // std::cerr << "[TEST_LOG] interpret() returned: " << (result ? "true" : "false") << " for expected FAIL. Output: " << output.str() << "--------------------\n" << std::endl;
                ASSERT_FALSE(interpret(input, output))
                    << "Code: " << code_str << "\nOutput: " << output.str();
                // Check that the error message doesn't accidentally contain kUnreachable
                // or if it does, that the program didn't actually print kUnreachable.
                // A more robust check might be to see if the error output is non-empty and doesn't contain kUnreachable.
                if (!output.str().empty()) { // If there's an error message
                    ASSERT_FALSE(ends_with(output.str(), kUnreachable));
                } else { // If interpret returned false but produced no output (should be an error string)
                    // This case might indicate an issue with error reporting itself.
                    // For now, let's assume an error string is always produced on failure.
                }
            }
        }
    }
}


TEST(IllegalOperationsSuite, ArgumentCountMismatch) {
    std::string code = R"(
        func = function(value) return 1 end function

        func(1, 2)

        print(239) // unreachable
    )";

    std::istringstream input(code);
    std::ostringstream output;

    ASSERT_FALSE(interpret(input, output));
    ASSERT_FALSE(ends_with(output.str(), kUnreachable));
}
