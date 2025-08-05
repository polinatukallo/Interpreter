#include "interpreter.h"
#include <iostream>
#include "parser.h"
#include "lexer.h"

// основная функция интерпретации кода
bool interpret(std::istream& input, std::ostream& output) {
    try {
        // чтение входного кода
        std::string code_content((std::istreambuf_iterator<char>(input)),
                          std::istreambuf_iterator<char>());
        
        // лексический анализ
        Lexer lexer(code_content);
        auto tokens = lexer.tokenize();
        
        // синтаксический анализ
        Parser parser(tokens);
        auto ast = parser.parse();

        // выполнение программы
        Interpreter interpreter_instance(output);
        interpreter_instance.interpret(ast);
        return true;
    } catch (const std::runtime_error& e) {
        output << "Runtime error (specific): " << e.what() << std::endl;
        return false;
    } catch (const std::exception& e) {
        output << "Runtime error (generic): " << e.what() << std::endl;
        return false;
    }
}


