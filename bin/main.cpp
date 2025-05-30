#include <iostream>
#include <sstream>
#include "interpreter.h" 

int main(int argc, char** argv) {
    std::string code = R"(
    max = function(arr)
    if len(arr) == 0 then
        return nil
    end if

    m = arr[0]

    for i in arr
        if i > m then m = i end if
    end for

    return m
end function

print(max([10, -1, 0, 2, 2025, 239]))
    )";

    std::istringstream input(code);
    std::ostringstream output;

    if (!interpret(input, output)) {
        std::cerr << "Error interpreting the code!" << std::endl;
        std::cerr << "Interpreter error message:\n" << output.str() << std::endl;
        return 1;
    }

    std::cout << "Interpreter output:\n" << output.str() << std::endl;

    return 0;
}
