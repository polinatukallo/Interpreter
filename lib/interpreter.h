#pragma once
#include <iostream>
#include <variant>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <cmath>
#include <algorithm>
#include <typeinfo>
#include <sstream>
#include <ctime>
#include <cctype>

#include "parser.h"
#include "lexer.h"

// основная функция интерпретации программы
bool interpret(std::istream& input, std::ostream& output);

// интерпретатор языка
class Interpreter {
    // структура для представления функции
    struct Function {
        std::vector<std::string> parameters;
        std::unique_ptr<Block> body;
        
        Function(std::vector<std::string> params, std::unique_ptr<Block> b)
            : parameters(std::move(params)), body(std::move(b)) {}
    };

    // структура для хранения значений различных типов
    struct Value {
        std::variant<double, std::string, std::shared_ptr<std::vector<Value>>, std::shared_ptr<Function>, std::nullptr_t> data;
        
        // конструкторы для разных типов значений
        Value() : data(std::nullptr_t{}) {} // значение по умолчанию - nil
        Value(double d) : data(d) {}
        Value(const std::string& s) : data(s) {}
        Value(std::string&& s) : data(std::move(s)) {}
        Value(const std::vector<Value>& v) : data(std::make_shared<std::vector<Value>>(v)) {}
        Value(std::vector<Value>&& v) : data(std::make_shared<std::vector<Value>>(std::move(v))) {}
        Value(std::shared_ptr<std::vector<Value>> pv) : data(std::move(pv)) {}
        Value(const std::shared_ptr<Function>& f) : data(f) {}
        Value(std::nullptr_t) : data(std::nullptr_t{}) {}
        
        // методы проверки типа значения
        bool isNumber() const { return std::holds_alternative<double>(data); } // является ли значение числом
        bool isString() const { return std::holds_alternative<std::string>(data); } // является ли значение строкой
        bool isList() const { return std::holds_alternative<std::shared_ptr<std::vector<Value>>>(data); } // является ли значение списком
        bool isFunction() const { return std::holds_alternative<std::shared_ptr<Function>>(data); } // является ли значение функцией
        bool isNil() const { return std::holds_alternative<std::nullptr_t>(data); } // является ли значение nil
        
        // методы приведения к конкретному типу
        double asNumber() const { // преобразование в число
            if (isNumber()) return std::get<double>(data);
            if (isNil()) return 0.0; // nil приводится к 0
            throw std::runtime_error("Cannot convert type " + typeName() + " to Number");
        }
        const std::string& asString() const { return std::get<std::string>(data); } // преобразование в строку
        std::vector<Value>& asList() { // получение изменяемой ссылки на список
            if (!isList()) throw std::runtime_error("Value is not a List, cannot call asList()");
            return *std::get<std::shared_ptr<std::vector<Value>>>(data);
        }
        const std::vector<Value>& asList() const { // получение константной ссылки на список
            if (!isList()) throw std::runtime_error("Value is not a List, cannot call asList()");
            return *std::get<std::shared_ptr<std::vector<Value>>>(data);
        }
        const std::shared_ptr<Function>& asFunction() const { return std::get<std::shared_ptr<Function>>(data); } // преобразование в функцию
        
        // преобразование значения в строку для вывода
        std::string toString() const {
            if (isNumber()) { // для чисел
                double num_val = std::get<double>(data);
                std::ostringstream oss;
                if (num_val == static_cast<long long>(num_val)) { // если число целое, выводим без десятичной части
                    oss << static_cast<long long>(num_val);
                } else {
                    oss.precision(15); // иначе с заданной точностью
                    oss << num_val;
                }
                return oss.str();
            }
            if (isString()) { // для строк
                std::string s_val = std::get<std::string>(data); 
                std::string escaped_s_repr = "\""; // экранируем специальные символы
                for (char c : s_val) {
                    switch (c) {
                        case '\\': escaped_s_repr += "\\\\"; break; 
                        case '"':  escaped_s_repr += "\\\""; break; 
                        case '\n': escaped_s_repr += "\\n"; break;  
                        case '\r': escaped_s_repr += "\\r"; break;  
                        case '\t': escaped_s_repr += "\\t"; break;  
                        default:   escaped_s_repr += c;     break;
                    }
                }
                escaped_s_repr += "\""; 
                return escaped_s_repr;
            }
            if (isList()) { // для списков
                std::string s_repr = "[";
                const auto& list_elements = *std::get<std::shared_ptr<std::vector<Value>>>(data);
                for (size_t i = 0; i < list_elements.size(); ++i) {
                    s_repr += list_elements[i].toString(); // рекурсивно вызываем toString для элементов
                    if (i < list_elements.size() - 1) {
                        s_repr += ", ";
                    }
                }
                s_repr += "]";
                return s_repr;
            }
            if (isFunction()) return "[function]"; // для функций
            if (isNil()) return "nil"; // для nil
            return "<unknown>"; // для неизвестных типов
        }
        
        // получение имени типа значения
        std::string typeName() const {
            if (isNumber()) return "Number";
            if (isString()) return "String";
            if (isList()) return "List";
            if (isFunction()) return "Function";
            if (isNil()) return "Nil";
            return "Unknown";
        }

        // преобразование в логическое значение (используется в условиях)
        bool asBool() const {
            if (isNumber()) return asNumber() != 0; // 0 - ложь, остальное - истина
            if (isString()) return !asString().empty(); // пустая строка - ложь
            if (isList()) return !asList().empty(); // пустой список - ложь
            if (isFunction()) return true; // функция всегда истина
            if (isNil()) return false; // nil всегда ложь
            return true; // остальные типы (если появятся) - истина по умолчанию
        }
    };

    // структура для возврата значения из функции (для реализации `return`)
    struct ReturnValue {
        Value value;
        explicit ReturnValue(Value v) : value(std::move(v)) {}
    };

    // сигналы для управления циклами
    struct BreakSignal {}; // сигнал для выхода из цикла (`break`)
    struct ContinueSignal {}; // сигнал для перехода к следующей итерации цикла (`continue`)

    // глобальные переменные и поток вывода
    std::unordered_map<std::string, Value> globals; // хранилище глобальных переменных
    std::ostream& output; // поток для вывода (например, `print`)

    public:
        // конструктор интерпретатора
        Interpreter(std::ostream& out) : output(out) {
            std::srand(static_cast<unsigned int>(std::time(0))); // инициализация генератора случайных чисел
            globals["len"] = Value{nullptr}; // предварительное объявление встроенной функции len
        }

        // выполнение программы (блока инструкций)
        void interpret(const std::unique_ptr<Block>& program) {
            try {
                for (const auto& stmt : program->statements) {
                    execute(stmt.get());
                }
            } catch (const ReturnValue& ret) {
            } catch (...) {
                throw;
            }
        }

    private:
        // вычисление значения выражения
        Value evaluate(Expression* expr) {
            if (!expr) { // проверка на нулевой указатель
                throw std::runtime_error("Null expression");
            }
            
            // определение типа узла выражения и его вычисление
            if (auto num = dynamic_cast<NumberLiteral*>(expr)) {
                return Value{num->value};
            }

            if (auto str = dynamic_cast<StringLiteral*>(expr)) {
                return Value{str->value};
            }

            if (auto nilLit = dynamic_cast<NilLiteral*>(expr)) {
                return Value{nullptr};
            }

            if (auto listLit = dynamic_cast<ListLiteral*>(expr)) { // создание списка
                std::vector<Value> values_vec;
                for (const auto& elemExpr : listLit->elements) {
                    values_vec.push_back(evaluate(elemExpr.get())); // вычисляем каждый элемент списка
                }
                return Value{std::make_shared<std::vector<Value>>(std::move(values_vec))};
            }

            if (auto funcDef = dynamic_cast<FunctionDefinition*>(expr)) { // определение функции
                return Value{std::make_shared<Function>(
                    funcDef->parameters,
                    std::unique_ptr<Block>(static_cast<Block*>(funcDef->body->clone())) // клонируем тело функции
                )};
            }

            if (auto call = dynamic_cast<FunctionCall*>(expr)) { // вызов функции
                if (!call->callee) {
                    throw std::runtime_error("Null function callee expression");
                }

                Identifier* calleeAsIdentifier = dynamic_cast<Identifier*>(call->callee.get());
                if (calleeAsIdentifier) { // если имя функции - идентификатор (не выражение)
                    const std::string& funcName = calleeAsIdentifier->name;
                    // обработка встроенных функций
                    if (funcName == "print") {
                        for (size_t i = 0; i < call->arguments.size(); ++i) {
                            const auto& arg_expr = call->arguments[i];
                            if (!arg_expr) {
                                throw std::runtime_error("Null argument in print");
                            }
                            Value val = evaluate(arg_expr.get());
                            if (val.isNumber()) {
                                double num_val_print = val.asNumber();
                                if (num_val_print == static_cast<long long>(num_val_print)) {
                                    output << static_cast<long long>(num_val_print);
                                } else {
                                    output.precision(15);
                                    output << num_val_print;
                                }
                            } else if (val.isString()) {
                                output << val.asString();
                            } else if (val.isNil()) {
                                output << "nil";
                            } else {
                                output << val.toString();
                            }
                        }
                        return Value{nullptr};
                    }
                    // функция len: возвращает длину строки или списка
                    if (funcName == "len") {
                        if (call->arguments.size() != 1) {
                            throw std::runtime_error("len() expects exactly 1 argument");
                        }
                        Value arg = evaluate(call->arguments[0].get());
                        if (arg.isString()) {
                            return Value{static_cast<double>(arg.asString().length())};
                        } else if (arg.isList()) {
                            return Value{static_cast<double>(arg.asList().size())};
                        }
                        throw std::runtime_error("len() argument must be a string or list");
                    }
                    // функция push: добавляет элемент в конец списка
                    if (funcName == "push") {
                        if (call->arguments.size() != 2) {
                            throw std::runtime_error("push() expects 2 arguments: list and value");
                        }
                        Value itemVal = evaluate(call->arguments[1].get());
                        
                        Identifier* listAsIdentifier = dynamic_cast<Identifier*>(call->arguments[0].get());
                        if (!listAsIdentifier) {
                            throw std::runtime_error("push() currently only supports pushing to lists stored in variables.");
                        }
                        auto it = globals.find(listAsIdentifier->name);
                        if (it == globals.end() || !it->second.isList()) {
                            throw std::runtime_error("Variable '" + listAsIdentifier->name + "' is not a list or not found for push().");
                        }
                        it->second.asList().push_back(itemVal);
                        return Value{nullptr}; // push возвращает nil
                    }
                    // функция pop: удаляет и возвращает последний элемент списка
                    if (funcName == "pop") {
                        if (call->arguments.size() != 1) {
                            throw std::runtime_error("pop() expects 1 argument: list");
                        }
                        Identifier* listAsIdentifier = dynamic_cast<Identifier*>(call->arguments[0].get());
                        if (!listAsIdentifier) {
                            throw std::runtime_error("pop() currently only supports popping from lists stored in variables.");
                        }
                        auto it = globals.find(listAsIdentifier->name);
                        if (it == globals.end() || !it->second.isList()) {
                            throw std::runtime_error("Variable '" + listAsIdentifier->name + "' is not a list or not found for pop().");
                        }
                        auto& vec = it->second.asList();
                        if (vec.empty()) {
                            throw std::runtime_error("Cannot pop from an empty list.");
                        }
                        Value val = vec.back();
                        vec.pop_back();
                        return val;
                    }
                    // функция insert: вставляет элемент в список по указанному индексу
                    if (funcName == "insert") {
                        if (call->arguments.size() != 3) {
                            throw std::runtime_error("insert() expects 3 arguments: list, index, value");
                        }
                        Value indexVal = evaluate(call->arguments[1].get());
                        Value itemVal = evaluate(call->arguments[2].get());

                        Identifier* listAsIdentifier = dynamic_cast<Identifier*>(call->arguments[0].get());
                        if (!listAsIdentifier) {
                            throw std::runtime_error("insert() currently only supports inserting into lists stored in variables.");
                        }
                        auto it = globals.find(listAsIdentifier->name);
                        if (it == globals.end() || !it->second.isList()) {
                            throw std::runtime_error("Variable '" + listAsIdentifier->name + "' is not a list or not found for insert().");
                        }

                        if (!indexVal.isNumber()) {
                            throw std::runtime_error("Second argument (index) to insert() must be a number. Got " + indexVal.typeName());
                        }
                        double raw_index = indexVal.asNumber();
                        if (raw_index != static_cast<long long>(raw_index)) {
                             throw std::runtime_error("List index for insert() must be an integer. Got " + std::to_string(raw_index));
                        }
                        long long idx = static_cast<long long>(raw_index);

                        auto& vec = it->second.asList();
                        if (idx < 0 || static_cast<size_t>(idx) > vec.size()) { 
                            throw std::runtime_error("Index out of bounds for insert(): " + std::to_string(idx) + ", size: " + std::to_string(vec.size()));
                        }
                        vec.insert(vec.begin() + idx, itemVal);
                        return Value{nullptr};
                    }
                    // функция remove: удаляет и возвращает элемент из списка по указанному индексу
                    if (funcName == "remove") {
                        if (call->arguments.size() != 2) {
                            throw std::runtime_error("remove() expects 2 arguments: list, index");
                        }
                        Value indexVal = evaluate(call->arguments[1].get());

                        Identifier* listAsIdentifier = dynamic_cast<Identifier*>(call->arguments[0].get());
                        if (!listAsIdentifier) {
                            throw std::runtime_error("remove() currently only supports removing from lists stored in variables.");
                        }
                        auto it = globals.find(listAsIdentifier->name);
                        if (it == globals.end() || !it->second.isList()) {
                            throw std::runtime_error("Variable '" + listAsIdentifier->name + "' is not a list or not found for remove().");
                        }

                        if (!indexVal.isNumber()) {
                            throw std::runtime_error("Second argument (index) to remove() must be a number. Got " + indexVal.typeName());
                        }
                        double raw_index = indexVal.asNumber();
                         if (raw_index != static_cast<long long>(raw_index)) {
                             throw std::runtime_error("List index for remove() must be an integer. Got " + std::to_string(raw_index));
                        }
                        long long idx = static_cast<long long>(raw_index);

                        auto& vec = it->second.asList();
                        if (idx < 0 || static_cast<size_t>(idx) >= vec.size()) {
                            throw std::runtime_error("Index out of bounds for remove(): " + std::to_string(idx) + ", size: " + std::to_string(vec.size()));
                        }
                        Value val = vec[static_cast<size_t>(idx)];
                        vec.erase(vec.begin() + idx);
                        return val;
                    }
                    // функция sort: сортирует список (чисел или строк)
                    if (funcName == "sort") {
                        if (call->arguments.size() != 1) {
                            throw std::runtime_error("sort() expects 1 argument: list");
                        }
                        Identifier* listAsIdentifier = dynamic_cast<Identifier*>(call->arguments[0].get());
                        if (!listAsIdentifier) {
                            throw std::runtime_error("sort() currently only supports sorting lists stored in variables.");
                        }
                        auto it = globals.find(listAsIdentifier->name);
                        if (it == globals.end() || !it->second.isList()) {
                            throw std::runtime_error("Variable '" + listAsIdentifier->name + "' is not a list or not found for sort().");
                        }
                        auto& vec = it->second.asList();
                        if (vec.empty()) return Value{nullptr};

                        bool sortNumbers = vec[0].isNumber();
                        bool sortStrings = vec[0].isString();

                        if (!sortNumbers && !sortStrings) {
                            throw std::runtime_error("sort() can only sort lists of numbers or lists of strings. First element type: " + vec[0].typeName());
                        }

                        std::sort(vec.begin(), vec.end(), [&](const Value& a, const Value& b) {
                            if (sortNumbers) {
                                if (!a.isNumber() || !b.isNumber()) throw std::runtime_error("Cannot sort list with mixed types (expected numbers).");
                                return a.asNumber() < b.asNumber();
                            } else {
                                if (!a.isString() || !b.isString()) throw std::runtime_error("Cannot sort list with mixed types (expected strings).");
                                return a.asString() < b.asString();
                            }
                        });
                        return Value{nullptr};
                    }
                    // функция range: создает список чисел в заданном диапазоне
                    if (funcName == "range") {
                        double start, stop_exclusive, step;
                        if (call->arguments.size() == 1) {
                            Value stopVal = evaluate(call->arguments[0].get());
                            if (!stopVal.isNumber()) throw std::runtime_error("range() single argument (stop) must be a number");
                            start = 0.0; stop_exclusive = stopVal.asNumber(); step = 1.0;
                        } else if (call->arguments.size() == 2) {
                            Value startVal = evaluate(call->arguments[0].get());
                            Value stopVal = evaluate(call->arguments[1].get());
                            if (!startVal.isNumber() || !stopVal.isNumber()) throw std::runtime_error("range() arguments (start, stop) must be numbers");
                            start = startVal.asNumber(); stop_exclusive = stopVal.asNumber(); step = 1.0;
                        } else if (call->arguments.size() == 3) {
                            Value startVal = evaluate(call->arguments[0].get());
                            Value stopVal = evaluate(call->arguments[1].get());
                            Value stepVal = evaluate(call->arguments[2].get());
                            if (!startVal.isNumber() || !stopVal.isNumber() || !stepVal.isNumber()) throw std::runtime_error("range() arguments (start, stop, step) must be numbers");
                            start = startVal.asNumber(); stop_exclusive = stopVal.asNumber(); step = stepVal.asNumber();
                        } else {
                            throw std::runtime_error("range() expects 1, 2, or 3 arguments");
                        }
                        if (step == 0) throw std::runtime_error("range() step argument cannot be zero");
                        auto rangeList_ptr = std::make_shared<std::vector<Value>>();
                        if (step > 0) { for (double i = start; i < stop_exclusive; i += step) rangeList_ptr->push_back(Value{i}); }
                        else { for (double i = start; i > stop_exclusive; i += step) rangeList_ptr->push_back(Value{i}); }
                        return Value{rangeList_ptr};
                    }
                    // функция abs: возвращает абсолютное значение числа
                    if (funcName == "abs") {
                        if (call->arguments.size() != 1) throw std::runtime_error("abs() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isNumber()) throw std::runtime_error("abs() argument must be a number");
                        return Value{std::fabs(arg.asNumber())};
                    }
                    // функция ceil: возвращает наименьшее целое число, не меньшее заданного
                    if (funcName == "ceil") {
                        if (call->arguments.size() != 1) throw std::runtime_error("ceil() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isNumber()) throw std::runtime_error("ceil() argument must be a number");
                        return Value{std::ceil(arg.asNumber())};
                    }
                    // функция floor: возвращает наибольшее целое число, не большее заданного
                    if (funcName == "floor") {
                        if (call->arguments.size() != 1) throw std::runtime_error("floor() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isNumber()) throw std::runtime_error("floor() argument must be a number");
                        return Value{std::floor(arg.asNumber())};
                    }
                    // функция round: округляет число до ближайшего целого
                    if (funcName == "round") {
                        if (call->arguments.size() != 1) throw std::runtime_error("round() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isNumber()) throw std::runtime_error("round() argument must be a number");
                        return Value{std::round(arg.asNumber())};
                    }
                    // функция sqrt: возвращает квадратный корень числа
                    if (funcName == "sqrt") {
                        if (call->arguments.size() != 1) throw std::runtime_error("sqrt() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isNumber()) throw std::runtime_error("sqrt() argument must be a number");
                        double num = arg.asNumber();
                        if (num < 0) throw std::runtime_error("sqrt() argument cannot be negative");
                        return Value{std::sqrt(num)};
                    }
                    // функция rnd: возвращает случайное число от 0.0 до 1.0
                    if (funcName == "rnd") { 
                        if (!call->arguments.empty()) throw std::runtime_error("rnd() expects 0 arguments");
                        return Value{static_cast<double>(std::rand()) / RAND_MAX};
                    }
                    // функция parse_num: преобразует строку в число, или nil при ошибке
                    if (funcName == "parse_num") {
                        if (call->arguments.size() != 1) throw std::runtime_error("parse_num() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isString()) {
                            throw std::runtime_error("parse_num() argument must be a string. Got " + arg.typeName());
                        }
                        try {
                            std::string str_to_parse = arg.asString();
                            size_t pos;
                            double result = std::stod(str_to_parse, &pos);
                            if (pos != str_to_parse.length()) {
                                return Value{nullptr};
                            }
                            return Value{result};
                        } catch (const std::invalid_argument& ia) {
                            return Value{nullptr};
                        } catch (const std::out_of_range& oor) {
                            return Value{nullptr};
                        }
                    }
                    // функция to_string: преобразует значение в строку
                    if (funcName == "to_string") {
                        if (call->arguments.size() != 1) throw std::runtime_error("to_string() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (arg.isNumber()) {
                            double num_val = arg.asNumber();
                            std::ostringstream oss;
                            if (num_val == static_cast<long long>(num_val)) {
                                oss << static_cast<long long>(num_val);
                            } else {
                                oss.precision(15); 
                                oss << num_val;
                            }
                            return Value{oss.str()};
                        }
                        if (arg.isString()) return Value{arg.asString()}; 
                        if (arg.isNil()) return Value{"nil"};
                        return Value{arg.toString()}; 
                    }
                    // функция lower: преобразует строку к нижнему регистру
                    if (funcName == "lower") {
                        if (call->arguments.size() != 1) throw std::runtime_error("lower() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isString()) throw std::runtime_error("lower() argument must be a string");
                        std::string s = arg.asString();
                        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                        return Value{s};
                    }
                    // функция upper: преобразует строку к верхнему регистру
                    if (funcName == "upper") {
                        if (call->arguments.size() != 1) throw std::runtime_error("upper() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isString()) throw std::runtime_error("upper() argument must be a string");
                        std::string s = arg.asString();
                        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
                        return Value{s};
                    }
                    // функция split: разбивает строку по разделителю на список строк
                    if (funcName == "split") {
                        if (call->arguments.size() != 2) throw std::runtime_error("split() expects 2 arguments: string and delimiter");
                        Value strVal = evaluate(call->arguments[0].get());
                        Value delimVal = evaluate(call->arguments[1].get());
                        if (!strVal.isString()) throw std::runtime_error("split() first argument must be a string");
                        if (!delimVal.isString()) throw std::runtime_error("split() second argument (delimiter) must be a string");
                        std::string s = strVal.asString();
                        std::string delimiter = delimVal.asString();
                        std::vector<Value> result;
                        size_t pos = 0;
                        std::string token;
                        if (delimiter.empty()) { // если разделитель пустой, разбиваем посимвольно
                            for (char c : s) {
                                result.push_back(Value{std::string(1, c)});
                            }
                        } else {
                            while ((pos = s.find(delimiter)) != std::string::npos) {
                                token = s.substr(0, pos);
                                result.push_back(Value{token});
                                s.erase(0, pos + delimiter.length());
                            }
                            result.push_back(Value{s});
                        }
                        return Value{result};
                    }
                    // функция join: объединяет список строк в одну строку с указанным разделителем
                    if (funcName == "join") {
                        if (call->arguments.size() != 2) throw std::runtime_error("join() expects 2 arguments: list_of_strings and separator");
                        Value listVal = evaluate(call->arguments[0].get());
                        Value sepVal = evaluate(call->arguments[1].get());
                        if (!listVal.isList()) throw std::runtime_error("join() first argument must be a list of strings");
                        if (!sepVal.isString()) throw std::runtime_error("join() second argument (separator) must be a string");
                        std::string separator = sepVal.asString();
                        std::string result_str;
                        const auto& strList = listVal.asList();
                        for (size_t i = 0; i < strList.size(); ++i) {
                            if (!strList[i].isString()) throw std::runtime_error("join() expects a list of strings; found non-string element: " + strList[i].typeName());
                            result_str += strList[i].asString();
                            if (i < strList.size() - 1) {
                                result_str += separator;
                            }
                        }
                        return Value{result_str};
                    }
                    // функция replace: заменяет все вхождения подстроки в строке на другую подстроку
                    if (funcName == "replace") {
                        if (call->arguments.size() != 3) throw std::runtime_error("replace() expects 3 arguments: string, old_substring, new_substring");
                        Value strVal = evaluate(call->arguments[0].get());
                        Value oldSubVal = evaluate(call->arguments[1].get());
                        Value newSubVal = evaluate(call->arguments[2].get());
                        if (!strVal.isString() || !oldSubVal.isString() || !newSubVal.isString()) {
                            throw std::runtime_error("replace() all arguments must be strings");
                        }
                        std::string s = strVal.asString();
                        std::string old_sub = oldSubVal.asString();
                        std::string new_sub = newSubVal.asString();
                        if (old_sub.empty()) {
                             throw std::runtime_error("replace() 'old_substring' cannot be empty."); 
                        }
                        size_t start_pos = 0;
                        while((start_pos = s.find(old_sub, start_pos)) != std::string::npos) {
                            s.replace(start_pos, old_sub.length(), new_sub);
                            start_pos += new_sub.length();
                        }
                        return Value{s};
                    }
                    // функция println: аналогична print, но добавляет перевод строки в конце
                    if (funcName == "println") {
                        for (size_t i = 0; i < call->arguments.size(); ++i) {
                            const auto& arg_expr = call->arguments[i];
                            if (!arg_expr) throw std::runtime_error("Null argument in println");
                            Value val = evaluate(arg_expr.get());
                             if (val.isNumber()) {
                                double num_val_print = val.asNumber();
                                if (num_val_print == static_cast<long long>(num_val_print)) {
                                    output << static_cast<long long>(num_val_print);
                                } else {
                                    output.precision(15);
                                    output << num_val_print;
                                }
                            } else if (val.isString()) {
                                output << val.asString();
                            } else if (val.isNil()) {
                                output << "nil";
                            } else {
                                output << val.toString();
                            }
                            if (i < call->arguments.size() - 1) {
                            }
                        }
                        output << std::endl;
                        return Value{nullptr};
                    }
                }

                Value funcValue = evaluate(call->callee.get()); // если callee - это выражение, вычисляем его

                if (!funcValue.isFunction()) { // проверяем, что результат - функция
                    std::string callee_repr = "expression";
                    if(calleeAsIdentifier) callee_repr = calleeAsIdentifier->name;
                    throw std::runtime_error("Attempted to call a non-function value (type: " + funcValue.typeName() + ") derived from: " + callee_repr );
                }

                auto actualFunc = funcValue.asFunction(); // получаем объект функции

                if (call->arguments.size() != actualFunc->parameters.size()) { // проверяем количество аргументов
                    throw std::runtime_error("Wrong number of arguments for function. Expected " + 
                                             std::to_string(actualFunc->parameters.size()) + ", Got " + std::to_string(call->arguments.size()));
                }

                std::unordered_map<std::string, Value> oldGlobals = globals; // сохраняем текущее состояние глобальных переменных (для рекурсии и изоляции)
                std::vector<Value> evaluated_args;
                for (const auto& arg_expr : call->arguments) { // вычисляем значения всех аргументов
                    evaluated_args.push_back(evaluate(arg_expr.get()));
                }
                for (size_t i = 0; i < actualFunc->parameters.size(); ++i) { // связываем параметры функции с вычисленными аргументами в глобальной области видимости (временно)
                    globals[actualFunc->parameters[i]] = evaluated_args[i];
                }

                try {
                    execute(actualFunc->body.get()); // выполняем тело функции
                    globals = oldGlobals; // восстанавливаем глобальные переменные
                    return Value{nullptr}; // по умолчанию функция возвращает nil, если нет явного return
                } catch (const ReturnValue& ret) { // если был `return` в функции
                    globals = oldGlobals; // восстанавливаем глобальные переменные
                    return ret.value; // возвращаем значение из ReturnValue
                }
            }

            if (auto unaryOp = dynamic_cast<UnaryOp*>(expr)) { // унарная операция
                Value operand = evaluate(unaryOp->operand.get());
                if (unaryOp->op == "not") { // логическое отрицание
                    return Value{static_cast<double>(!operand.asBool())};
                } else if (unaryOp->op == "-") { // унарный минус
                    if (!operand.isNumber()) {
                        throw std::runtime_error("Operand for unary '-' must be a number. Got " + operand.typeName());
                    }
                    return Value{-operand.asNumber()};
                }
                throw std::runtime_error("Unknown unary operator: " + unaryOp->op);
            }

            if (auto indexAccess = dynamic_cast<IndexAccess*>(expr)) { // доступ по индексу или срез
                Value target = evaluate(indexAccess->target.get()); // вычисляем целевой объект (строку или список)

                if (indexAccess->index) { // если это доступ по одному индексу
                    Value index_val = evaluate(indexAccess->index.get());
                    if (!index_val.isNumber()) {
                        throw std::runtime_error("Index must be a number. Got " + index_val.typeName());
                    }
                    double raw_index = index_val.asNumber();
                    if (raw_index != static_cast<long long>(raw_index)) { // индекс должен быть целым числом
                        throw std::runtime_error("Index must be an integer. Got " + std::to_string(raw_index));
                    }
                    long long int_index = static_cast<long long>(raw_index);

                    if (target.isString()) { // доступ к символу строки
                        const std::string& str = target.asString();
                        long long len = static_cast<long long>(str.length());
                        if (int_index < 0) int_index += len; // поддержка отрицательных индексов
                        if (int_index < 0 || int_index >= len) { // проверка выхода за границы
                            throw std::runtime_error("String index out of bounds: " + std::to_string(int_index - (int_index >= len ? len : 0)) + ", size: " + std::to_string(len));
                        }
                        return Value{std::string(1, str[static_cast<size_t>(int_index)])};
                    } else if (target.isList()) { // доступ к элементу списка
                        const auto& list = target.asList();
                        long long len = static_cast<long long>(list.size());
                        if (int_index < 0) int_index += len; // поддержка отрицательных индексов
                        if (int_index < 0 || int_index >= len) { // проверка выхода за границы
                            throw std::runtime_error("List index out of bounds: " + std::to_string(int_index - (int_index >= len ? len : 0)) + ", size: " + std::to_string(len));
                        }
                        return list[static_cast<size_t>(int_index)];
                    } else {
                        throw std::runtime_error("Cannot apply index operator to type " + target.typeName());
                    }
                } else { // если это срез (slice)
                    long long start = 0, end = 0, step = 1; // значения по умолчанию для среза
                    // вычисление начального индекса среза (если есть)
                    if (indexAccess->start_slice) {
                        Value startVal = evaluate(indexAccess->start_slice.get());
                        if (!startVal.isNumber()) throw std::runtime_error("Slice start index must be a number.");
                        start = static_cast<long long>(startVal.asNumber());
                    }
                    // вычисление конечного индекса среза (если есть)
                    if (indexAccess->end_slice) {
                        Value endVal = evaluate(indexAccess->end_slice.get());
                        if (!endVal.isNumber()) throw std::runtime_error("Slice end index must be a number.");
                        end = static_cast<long long>(endVal.asNumber());
                    }
                    // вычисление шага среза (если есть)
                    if (indexAccess->step_slice) {
                        Value stepVal = evaluate(indexAccess->step_slice.get());
                        if (!stepVal.isNumber()) throw std::runtime_error("Slice step must be a number.");
                        step = static_cast<long long>(stepVal.asNumber());
                        if (step == 0) throw std::runtime_error("Slice step cannot be zero.");
                    }
                    
                    // обработка среза для списка (пока только для списков строк, возвращает строку)
                    long long len = static_cast<long long>(target.asList().size());
                    // нормализация индексов (обработка отрицательных, приведение к границам списка)
                    if (start < 0) start += len;
                    if (end < 0) end += len;
                    start = std::max(0LL, start);
                    end = std::max(0LL, end);
                    start = std::min(len, start);
                    end = std::min(len, end);

                    std::string result_str; // результат среза (пока всегда строка)
                    if (step > 0) { // прямой порядок
                        for (long long i = start; i < end; i += step) {
                            result_str += target.asList()[static_cast<size_t>(i)].asString();
                        }
                    } else { // обратный порядок
                        for (long long i = start; i > end; i += step) {
                            if (i < len && i >= 0) { // проверка, чтобы не выйти за реальные границы при отрицательном шаге
                                result_str += target.asList()[static_cast<size_t>(i)].asString();
                            }
                        }
                    }
                    return Value{result_str};
                }
            }

            if (auto assign = dynamic_cast<Assignment*>(expr)) { // операция присваивания
                if (!assign->value) {
                    throw std::runtime_error("Null value in assignment to " + assign->name);
                }
                Value val_to_assign = evaluate(assign->value.get()); // вычисляем правую часть присваивания

                if (assign->op == "=") { // простое присваивание
                    globals[assign->name] = val_to_assign;
                    return val_to_assign;
                } else { // составное присваивание (+=, -=, *=, /=, %=, ^=)
                    // ... (логика составных присваиваний с комментариями ниже)
                    auto it = globals.find(assign->name);
                    if (it == globals.end()) { // переменная должна быть определена
                        throw std::runtime_error("Undefined variable for compound assignment: " + assign->name);
                    }
                    Value current_val = it->second;
                    Value result_val{nullptr}; // инициализация по умолчанию

                    // определяем бинарную операцию из оператора присваивания (например, "+=" -> "+")
                    std::string binary_op_str = assign->op.substr(0, assign->op.length() - 1); 
                    if (binary_op_str == "+") { // сложение или конкатенация
                        if (current_val.isNumber() && val_to_assign.isNumber()) {
                            result_val = Value{current_val.asNumber() + val_to_assign.asNumber()};
                        } else if (current_val.isString() && val_to_assign.isString()) {
                            result_val = Value{current_val.asString() + val_to_assign.asString()};
                        } else if (current_val.isList() && val_to_assign.isList()) {
                            auto result_list_vec = current_val.asList(); // копия списка
                            const auto& right_list_vec = val_to_assign.asList();
                            result_list_vec.insert(result_list_vec.end(), right_list_vec.begin(), right_list_vec.end()); // добавляем элементы второго списка
                            result_val = Value{result_list_vec};
                        } else {
                            throw std::runtime_error("Operator '" + assign->op + "' cannot be applied to types " + current_val.typeName() + " and " + val_to_assign.typeName());
                        }
                    } else if (binary_op_str == "-") { // вычитание
                        if (current_val.isNumber() && val_to_assign.isNumber()) {
                            result_val = Value{current_val.asNumber() - val_to_assign.asNumber()};
                        } else if (current_val.isString() && val_to_assign.isString()) {
                            // вычитание строки (удаление суффикса)
                            const std::string& main_str = current_val.asString();
                            const std::string& suffix_str = val_to_assign.asString();
                            if (main_str.length() >= suffix_str.length() && 
                                main_str.substr(main_str.length() - suffix_str.length()) == suffix_str) {
                                result_val = Value{main_str.substr(0, main_str.length() - suffix_str.length())};
                            } else {
                                result_val = Value{main_str}; // если не суффикс, возвращаем исходную строку
                            }
                        } else {
                            throw std::runtime_error("Operator '-' cannot be applied to types " + current_val.typeName() + " and " + val_to_assign.typeName());
                        }
                    } else if (binary_op_str == "*") { // умножение или повторение
                         if (current_val.isNumber() && val_to_assign.isNumber()) {
                            result_val = Value{current_val.asNumber() * val_to_assign.asNumber()};
                        } else if ((current_val.isString() && val_to_assign.isNumber()) || (current_val.isNumber() && val_to_assign.isString())) {
                            const std::string* str_val_ptr;
                            double num_val;
                            // определяем, какой операнд строка, какой число
                            if (current_val.isString()) { str_val_ptr = &current_val.asString(); num_val = val_to_assign.asNumber(); }
                            else { str_val_ptr = &val_to_assign.asString(); num_val = current_val.asNumber(); } // это неверно для var *= num, исправлено ниже
                            if (current_val.isString() && val_to_assign.isNumber()) { // строка *= число
                                str_val_ptr = &current_val.asString(); num_val = val_to_assign.asNumber();
                            } else if (current_val.isNumber() && val_to_assign.isString()) { // число *= строка не поддерживается
                                 throw std::runtime_error("Operator '*=' with number on left and string on right is not supported.");
                            } else {
                                throw std::runtime_error("Internal error or invalid types for '*='");
                            }

                            if (num_val < 0) throw std::runtime_error("Cannot multiply string by negative number for '*='");
                            std::string temp_result_str;
                            for (int i = 0; i < static_cast<int>(num_val); ++i) { temp_result_str += *str_val_ptr; } // повторяем строку num_val раз
                            result_val = Value{temp_result_str};

                        } else if ((current_val.isList() && val_to_assign.isNumber())) { // список *= число
                            const auto& list_val_vec = current_val.asList();
                            double num_val = val_to_assign.asNumber();
                            if (num_val < 0) throw std::runtime_error("Cannot multiply list by negative number for '*='");
                            std::vector<Value> temp_result_list;
                            for (int i = 0; i < static_cast<int>(num_val); ++i) { // повторяем элементы списка num_val раз
                                temp_result_list.insert(temp_result_list.end(), list_val_vec.begin(), list_val_vec.end()); 
                            }
                            result_val = Value{temp_result_list};
                        } else {
                            throw std::runtime_error("Operator '" + assign->op + "' cannot be applied to types " + current_val.typeName() + " and " + val_to_assign.typeName());
                        }
                    } else if (binary_op_str == "/") { // деление
                        if (current_val.isNumber() && val_to_assign.isNumber()) {
                            if (val_to_assign.asNumber() == 0) throw std::runtime_error("Division by zero for '/='");
                            result_val = Value{current_val.asNumber() / val_to_assign.asNumber()};
                        } else {
                            throw std::runtime_error("Operator '" + assign->op + "' requires numeric operands");
                        }
                    } else if (binary_op_str == "%") { // остаток от деления
                        if (current_val.isNumber() && val_to_assign.isNumber()) {
                            if (val_to_assign.asNumber() == 0) throw std::runtime_error("Modulo by zero for '%='");
                            result_val = Value{std::fmod(current_val.asNumber(), val_to_assign.asNumber())};
                        } else {
                            throw std::runtime_error("Operator '" + assign->op + "' requires numeric operands");
                        }
                    } else if (binary_op_str == "^") { // возведение в степень
                         if (current_val.isNumber() && val_to_assign.isNumber()) {
                            result_val = Value{std::pow(current_val.asNumber(), val_to_assign.asNumber())};
                        } else {
                            throw std::runtime_error("Operator '" + assign->op + "' requires numeric operands");
                        }
                    } else {
                        throw std::runtime_error("Unsupported compound assignment operator: " + assign->op);
                    }
                    
                    globals[assign->name] = result_val; // обновляем значение переменной
                    return result_val;
                }
            }

            if (auto id = dynamic_cast<Identifier*>(expr)) { // идентификатор (переменная или имя функции)
                // проверка, не пытаемся ли мы получить значение встроенной функции без вызова
                if (id->name == "print" || id->name == "range" || id->name == "len" ||
                    id->name == "push" || id->name == "pop" || id->name == "insert" ||
                    id->name == "remove" || id->name == "sort" || id->name == "abs" ||
                    id->name == "ceil" || id->name == "floor" || id->name == "round" ||
                    id->name == "sqrt" || id->name == "rnd" || id->name == "parse_num" ||
                    id->name == "to_string" || id->name == "lower" || id->name == "upper" ||
                    id->name == "split" || id->name == "join" || id->name == "replace" ||
                    id->name == "println" 
                ) {
                    throw std::runtime_error("Built-in function '" + id->name + "' must be called with parentheses ()");
                }
                
                auto it = globals.find(id->name);
                if (it == globals.end()) { // переменная не найдена
                    throw std::runtime_error("Undefined variable: " + id->name);
                }
                return it->second;
            }

            if (auto binOp = dynamic_cast<BinaryOp*>(expr)) { // бинарная операция
                auto left = evaluate(binOp->left.get()); // вычисляем левый операнд
                auto right = evaluate(binOp->right.get()); // вычисляем правый операнд

                if (binOp->op == "+") { // сложение или конкатенация
                    if (left.isNil() || right.isNil()) { // сложение с nil не допускается
                        throw std::runtime_error("Operator '+' cannot be applied if an operand is Nil");
                    }
                    if (left.isNumber() && right.isNumber()) {
                        return Value{left.asNumber() + right.asNumber()};
                    } else if (left.isString() && right.isString()) {
                        return Value{left.asString() + right.asString()};
                    } else if (left.isList() && right.isList()) {
                        auto result_list = left.asList(); // создаем копию левого списка
                        const auto& right_list = right.asList();
                        result_list.insert(result_list.end(), right_list.begin(), right_list.end()); // добавляем элементы правого списка
                        return Value{result_list};
                    } else {
                        throw std::runtime_error("Operator '+' cannot be applied to types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "-") { // вычитание
                    if (left.isNil() || right.isNil()) { // вычитание с nil не допускается
                        throw std::runtime_error("Operator '-' cannot be applied if an operand is Nil");
                    }
                    if (left.isNumber() && right.isNumber()) {
                        return Value{left.asNumber() - right.asNumber()};
                    } else if (left.isString() && right.isString()) {
                        // вычитание строки (удаление суффикса)
                        const std::string& main_str = left.asString();
                        const std::string& suffix_str = right.asString();
                        if (main_str.length() >= suffix_str.length() && 
                            main_str.substr(main_str.length() - suffix_str.length()) == suffix_str) {
                            return Value{main_str.substr(0, main_str.length() - suffix_str.length())};
                        } else {
                            return Value{main_str}; // если не суффикс, возвращаем исходную строку
                        }
                    } else {
                        throw std::runtime_error("Operator '-' cannot be applied to types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "/" || binOp->op == "%" || binOp->op == "^") { // деление, остаток, степень (только для чисел)
                    if (left.isNil() || right.isNil()) {
                        throw std::runtime_error("Operator '" + binOp->op + "' cannot be applied if an operand is Nil");
                    }
                    if (left.isNumber() && right.isNumber()) {
                        double num_left = left.asNumber();
                        double num_right = right.asNumber(); 
                        if ((binOp->op == "/" || binOp->op == "%") && num_right == 0.0) { // проверка деления на ноль
                            throw std::runtime_error(binOp->op == "/" ? "Division by zero" : "Modulo by zero");
                        }
                        if (binOp->op == "/") return Value{num_left / num_right};
                        if (binOp->op == "%") return Value{std::fmod(num_left, num_right)};
                        if (binOp->op == "^") return Value{std::pow(num_left, num_right)};
                    } else {
                        throw std::runtime_error("Operator '" + binOp->op + "' cannot be applied to types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "*") { // умножение (чисел) или повторение (строка/список * число)
                    if (left.isNumber() && right.isNumber()) {
                        return Value{left.asNumber() * right.asNumber()};
                    } else if ((left.isString() && right.isNumber()) || (left.isNumber() && right.isString())) { // строка * число или число * строка
                        const std::string* str_val;
                        double num_val;
                        if (left.isString()) { str_val = &left.asString(); num_val = right.asNumber(); }
                        else { str_val = &right.asString(); num_val = left.asNumber(); } // число * строка
                        if (num_val < 0) throw std::runtime_error("Cannot multiply string by negative number");
                        std::string result_str;
                        for (int i = 0; i < static_cast<int>(num_val); ++i) { result_str += *str_val; } // повторяем строку
                        return Value{result_str};
                    } else if ((left.isList() && right.isNumber()) || (left.isNumber() && right.isList())) { // список * число или число * список
                        const std::vector<Value>* list_val;
                        double num_val;
                        if (left.isList()) { list_val = &left.asList(); num_val = right.asNumber(); }
                        else { list_val = &right.asList(); num_val = left.asNumber(); } // число * список
                        if (num_val < 0) throw std::runtime_error("Cannot multiply list by negative number");
                        std::vector<Value> result_list;
                        for (int i = 0; i < static_cast<int>(num_val); ++i) { result_list.insert(result_list.end(), list_val->begin(), list_val->end()); } // повторяем элементы списка
                        return Value{result_list};
                    } else {
                        throw std::runtime_error("Operator '*' cannot be applied to types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "==" || binOp->op == "!=") { // равенство/неравенство
                    if (left.isNil() && right.isNil()) return Value{static_cast<double>(binOp->op == "==")}; // nil == nil -> true
                    if (left.isNil() || right.isNil()) return Value{static_cast<double>(binOp->op == "!=")}; // nil == non-nil -> false (для ==)
                    if (left.isNumber() && right.isNumber()) {
                        if (binOp->op == "==") return Value{static_cast<double>(left.asNumber() == right.asNumber())};
                        if (binOp->op == "!=") return Value{static_cast<double>(left.asNumber() != right.asNumber())};
                    } else if (left.isString() && right.isString()) {
                        if (binOp->op == "==") return Value{static_cast<double>(left.asString() == right.asString())};
                        if (binOp->op == "!=") return Value{static_cast<double>(left.asString() != right.asString())};
                    } else { // сравнение других типов (например, число со строкой) на равенство/неравенство не поддерживается
                        throw std::runtime_error("Operator '" + binOp->op + "' cannot compare types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "<" || binOp->op == ">" || binOp->op == "<=" || binOp->op == ">=") { // сравнение порядка
                    if (left.isNil() || right.isNil()) { // сравнение с nil не допускается
                        throw std::runtime_error("Operator '" + binOp->op + "' cannot be applied if an operand is Nil");
                    }
                    if (left.isNumber() && right.isNumber()) {
                        double l = left.asNumber(), r = right.asNumber();
                        if (binOp->op == "<") return Value{static_cast<double>(l < r)};
                        if (binOp->op == ">") return Value{static_cast<double>(l > r)};
                        if (binOp->op == "<=") return Value{static_cast<double>(l <= r)};
                        if (binOp->op == ">=") return Value{static_cast<double>(l >= r)};
                    } else if (left.isString() && right.isString()) { // строки сравниваются лексикографически
                        const std::string& l = left.asString(), &r = right.asString();
                        if (binOp->op == "<") return Value{static_cast<double>(l < r)};
                        if (binOp->op == ">") return Value{static_cast<double>(l > r)};
                        if (binOp->op == "<=") return Value{static_cast<double>(l <= r)};
                        if (binOp->op == ">=") return Value{static_cast<double>(l >= r)};
                    } else {
                        throw std::runtime_error("Operator '" + binOp->op + "' cannot be applied to types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "and" || binOp->op == "or") { // логические операции
                    if (binOp->op == "and") return Value{static_cast<double>(left.asBool() && right.asBool())}; // используется приведение к bool
                    if (binOp->op == "or") return Value{static_cast<double>(left.asBool() || right.asBool())}; // используется приведение к bool
                }
                throw std::runtime_error("Unknown operator '" + binOp->op + "' for types " + left.typeName() + " and " + right.typeName());
            }

            throw std::runtime_error("Unknown expression type in evaluate: " + std::string(typeid(*expr).name()));
        }

    // выполнение узла аст (инструкции или блока)
    Value execute(ASTNode* node) {
        if (auto returnStmt = dynamic_cast<ReturnStatement*>(node)) { // инструкция return
            if (returnStmt->value) { // если есть возвращаемое выражение
                throw ReturnValue(evaluate(returnStmt->value.get())); // выбрасываем специальный объект ReturnValue
            } else {
                throw ReturnValue(Value{nullptr}); // если return без выражения, возвращаем nil
            }
        }
        if (auto breakStmt = dynamic_cast<BreakStatement*>(node)) { // инструкция break
            throw BreakSignal(); // выбрасываем сигнал BreakSignal
        }
        if (auto continueStmt = dynamic_cast<ContinueStatement*>(node)) { // инструкция continue
            throw ContinueSignal(); // выбрасываем сигнал ContinueSignal
        }
        if (auto exprStmt = dynamic_cast<ExpressionStatement*>(node)) { // инструкция-выражение
            return evaluate(exprStmt->expression.get()); // вычисляем выражение, результат игнорируется (кроме побочных эффектов)
        }
        if (auto expr = dynamic_cast<Expression*>(node)) { // если узел сам по себе выражение 
            return evaluate(expr);
        }
        if (auto block = dynamic_cast<Block*>(node)) { // блок кода
            Value lastValue{nullptr}; // значение последней выполненной инструкции в блоке
            for (const auto& stmt : block->statements) {
                lastValue = execute(stmt.get()); // выполняем каждую инструкцию
            }
            return lastValue; // блок возвращает значение своей последней инструкции (важно для REPL)
        }
        if (auto ifStmt = dynamic_cast<IfStatement*>(node)) { // условный оператор if
            Value condValue = evaluate(ifStmt->condition.get()); // вычисляем условие
            
            if (condValue.asBool()) { // если условие истинно
                return execute(ifStmt->thenBlock.get()); // выполняем блок then
            } else { // иначе, проверяем блоки else if
                for (auto& elseIf : ifStmt->elseIfBlocks) {
                    Value elseIfCondValue = evaluate(elseIf.first.get());
                    if (elseIfCondValue.asBool()) { // если условие else if истинно
                        return execute(elseIf.second.get()); // выполняем соответствующий блок
                    }
                }
                if (ifStmt->elseBlock) { // если есть блок else и все предыдущие условия ложны
                    return execute(ifStmt->elseBlock.get()); // выполняем блок else
                }
            }
            return Value{nullptr}; // если ни один блок не был выполнен, if возвращает nil
        }
        if (auto whileStmt = dynamic_cast<WhileStatement*>(node)) { // цикл while
            while (evaluate(whileStmt->condition.get()).asBool()) { // пока условие истинно
                try {
                    execute(whileStmt->body.get()); // выполняем тело цикла
                } catch (const BreakSignal&) { // если был break
                    break; // выходим из цикла while
                } catch (const ContinueSignal&) { // если был continue
                    continue; // переходим к следующей итерации
                }
            }
            return Value{nullptr}; // цикл while возвращает nil
        }
        if (auto forStmt = dynamic_cast<ForStatement*>(node)) { // цикл for
            // специальная оптимизация для for ... in range(...)
            if (auto call = dynamic_cast<FunctionCall*>(forStmt->iterable.get())) {
                if (auto id = dynamic_cast<Identifier*>(call->callee.get())) {
                    if (id->name == "range") {
                        Value rangeValue = evaluate(call); // вычисляем range()
                        const auto& list = rangeValue.asList(); 
                        for (const auto& item : list) { // итерируемся по сгенерированному списку
                            globals[forStmt->variableName] = item; // присваиваем значение переменной цикла
                            try {
                                execute(forStmt->body.get()); // выполняем тело цикла
                            } catch (const BreakSignal&) {
                                goto end_for_loop; // выход из цикла for по break
                            } catch (const ContinueSignal&) {
                                // переход к следующей итерации (continue)
                            }
                        }
                        end_for_loop:; // метка для goto
                        return Value{nullptr};
                    }
                }
            }
            
            Value iterableValue = evaluate(forStmt->iterable.get()); // вычисляем итерируемый объект

            if (iterableValue.isList()) { // итерация по списку
                const auto& list = iterableValue.asList();
                for (const auto& item : list) {
                    globals[forStmt->variableName] = item;
                    try {
                        execute(forStmt->body.get());
                    } catch (const BreakSignal&) {
                        goto end_for_list_iteration; 
                    } catch (const ContinueSignal&) {
                    }
                }
                end_for_list_iteration:;
            } else if (iterableValue.isString()) { // итерация по строке (посимвольно)
                const std::string& str = iterableValue.asString();
                for (char ch : str) {
                    globals[forStmt->variableName] = Value{std::string(1, ch)}; // каждый символ - строка длины 1
                    try {
                        execute(forStmt->body.get());
                    } catch (const BreakSignal&) {
                        goto end_for_string_iteration;
                    } catch (const ContinueSignal&) {
                    }
                }
                end_for_string_iteration:;
            } else {
                throw std::runtime_error("For loop can only iterate over lists or strings. Got: " + iterableValue.typeName());
            }
            return Value{nullptr}; // цикл for возвращает nil
        }
        throw std::runtime_error("Unknown ASTNode type: " + std::string(typeid(*node).name()));
        return Value{nullptr};
    }
};


    