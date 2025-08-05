#include <iostream>
#include <sstream>
#include "interpreter.h" 

int main(int argc, char** argv) {
    std::string code = R"(
    
    println("Enter here your code")
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
