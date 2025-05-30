#include "interpreter.h"
#include <iostream>
#include "parser.h"
#include "lexer.h"

bool interpret(std::istream& input, std::ostream& output) {
    try {
        // чтение входного кода из потока ввода
        std::string code_content((std::istreambuf_iterator<char>(input)),
                          std::istreambuf_iterator<char>());
        
        // лексический анализ, разбиение кода на токены
        Lexer lexer(code_content);
        auto tokens = lexer.tokenize();
        
        // синтаксический анализ, построение аст
        Parser parser(tokens);
        auto ast = parser.parse();

        // выполнение программы, интерпретация аст
        Interpreter interpreter_instance(output); // создание экземпляра интерпретатора с указанием потока вывода
        interpreter_instance.interpret(ast);
        return true; 
    } catch (const std::runtime_error& e) {
        output << "Runtime error (specific): " << e.what() << std::endl; // вывод специфической ошибки времени выполнения
        return false; 
    } catch (const std::exception& e) {
        output << "Runtime error (generic): " << e.what() << std::endl; // вывод общей ошибки времени выполнения
        return false; 
    }
}


