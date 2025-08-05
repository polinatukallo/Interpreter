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
        Value() : data(std::nullptr_t{}) {}
        Value(double d) : data(d) {}
        Value(const std::string& s) : data(s) {}
        Value(std::string&& s) : data(std::move(s)) {}
        Value(const std::vector<Value>& v) : data(std::make_shared<std::vector<Value>>(v)) {}
        Value(std::vector<Value>&& v) : data(std::make_shared<std::vector<Value>>(std::move(v))) {}
        Value(std::shared_ptr<std::vector<Value>> pv) : data(std::move(pv)) {}
        Value(const std::shared_ptr<Function>& f) : data(f) {}
        Value(std::nullptr_t) : data(std::nullptr_t{}) {}
        
        // методы проверки типа значения
        bool isNumber() const { return std::holds_alternative<double>(data); }
        bool isString() const { return std::holds_alternative<std::string>(data); }
        bool isList() const { return std::holds_alternative<std::shared_ptr<std::vector<Value>>>(data); }
        bool isFunction() const { return std::holds_alternative<std::shared_ptr<Function>>(data); }
        bool isNil() const { return std::holds_alternative<std::nullptr_t>(data); }
        
        // методы приведения к конкретному типу
        double asNumber() const { 
            if (isNumber()) return std::get<double>(data);
            if (isNil()) return 0.0;
            throw std::runtime_error("Cannot convert type " + typeName() + " to Number");
        }
        const std::string& asString() const { return std::get<std::string>(data); }
        std::vector<Value>& asList() { 
            if (!isList()) throw std::runtime_error("Value is not a List, cannot call asList()");
            return *std::get<std::shared_ptr<std::vector<Value>>>(data);
        }
        const std::vector<Value>& asList() const { 
            if (!isList()) throw std::runtime_error("Value is not a List, cannot call asList()");
            return *std::get<std::shared_ptr<std::vector<Value>>>(data);
        }
        const std::shared_ptr<Function>& asFunction() const { return std::get<std::shared_ptr<Function>>(data); }
        
        // преобразование значения в строку для вывода
        std::string toString() const {
            if (isNumber()) {
                double num_val = std::get<double>(data);
                std::ostringstream oss;
                if (num_val == static_cast<long long>(num_val)) {
                    oss << static_cast<long long>(num_val);
                } else {
                    oss.precision(15);
                    oss << num_val;
                }
                return oss.str();
            }
            if (isString()) {
                std::string s_val = std::get<std::string>(data); 
                std::string escaped_s_repr = "\""; 
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
            if (isList()) { 
                std::string s_repr = "[";
                const auto& list_elements = *std::get<std::shared_ptr<std::vector<Value>>>(data);
                for (size_t i = 0; i < list_elements.size(); ++i) {
                    s_repr += list_elements[i].toString();
                    if (i < list_elements.size() - 1) {
                        s_repr += ", ";
                    }
                }
                s_repr += "]";
                return s_repr;
            }
            if (isFunction()) return "[function]";
            if (isNil()) return "nil";
            return "<unknown>";
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

        // преобразование в логическое значение
        bool asBool() const {
            if (isNumber()) return asNumber() != 0;
            if (isString()) return !asString().empty();
            if (isList()) return !asList().empty();
            if (isFunction()) return true;
            if (isNil()) return false;
            return true;
        }
    };

    // структура для возврата значения из функции
    struct ReturnValue {
        Value value;
        explicit ReturnValue(Value v) : value(std::move(v)) {}
    };

    // сигналы для управления циклами
    struct BreakSignal {};
    struct ContinueSignal {};

    // глобальные переменные и поток вывода
    std::unordered_map<std::string, Value> globals;
    std::ostream& output;

    public:
        // конструктор интерпретатора
        Interpreter(std::ostream& out) : output(out) {
            std::srand(static_cast<unsigned int>(std::time(0)));
            globals["len"] = Value{nullptr};
        }

        // выполнение программы
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
            if (!expr) {
                throw std::runtime_error("Null expression");
            }
            
            if (auto num = dynamic_cast<NumberLiteral*>(expr)) {
                return Value{num->value};
            }

            if (auto str = dynamic_cast<StringLiteral*>(expr)) {
                return Value{str->value};
            }

            if (auto nilLit = dynamic_cast<NilLiteral*>(expr)) {
                return Value{nullptr};
            }

            if (auto listLit = dynamic_cast<ListLiteral*>(expr)) {
                std::vector<Value> values_vec;
                for (const auto& elemExpr : listLit->elements) {
                    values_vec.push_back(evaluate(elemExpr.get()));
                }
                return Value{std::make_shared<std::vector<Value>>(std::move(values_vec))};
            }

            if (auto funcDef = dynamic_cast<FunctionDefinition*>(expr)) {
                return Value{std::make_shared<Function>(
                    funcDef->parameters,
                    std::unique_ptr<Block>(static_cast<Block*>(funcDef->body->clone()))
                )};
            }

            if (auto call = dynamic_cast<FunctionCall*>(expr)) {
                if (!call->callee) {
                    throw std::runtime_error("Null function callee expression");
                }

                Identifier* calleeAsIdentifier = dynamic_cast<Identifier*>(call->callee.get());
                if (calleeAsIdentifier) {
                    const std::string& funcName = calleeAsIdentifier->name;
                    // встроенная функция print
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
                    // встроенная функция len
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
                    // встроенная функция push
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
                        return Value{nullptr}; // push returns nil
                    }
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
                    if (funcName == "abs") {
                        if (call->arguments.size() != 1) throw std::runtime_error("abs() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isNumber()) throw std::runtime_error("abs() argument must be a number");
                        return Value{std::fabs(arg.asNumber())};
                    }
                    if (funcName == "ceil") {
                        if (call->arguments.size() != 1) throw std::runtime_error("ceil() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isNumber()) throw std::runtime_error("ceil() argument must be a number");
                        return Value{std::ceil(arg.asNumber())};
                    }
                    if (funcName == "floor") {
                        if (call->arguments.size() != 1) throw std::runtime_error("floor() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isNumber()) throw std::runtime_error("floor() argument must be a number");
                        return Value{std::floor(arg.asNumber())};
                    }
                    if (funcName == "round") {
                        if (call->arguments.size() != 1) throw std::runtime_error("round() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isNumber()) throw std::runtime_error("round() argument must be a number");
                        return Value{std::round(arg.asNumber())};
                    }
                    if (funcName == "sqrt") {
                        if (call->arguments.size() != 1) throw std::runtime_error("sqrt() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isNumber()) throw std::runtime_error("sqrt() argument must be a number");
                        double num = arg.asNumber();
                        if (num < 0) throw std::runtime_error("sqrt() argument cannot be negative");
                        return Value{std::sqrt(num)};
                    }
                    if (funcName == "rnd") { 
                        if (!call->arguments.empty()) throw std::runtime_error("rnd() expects 0 arguments");
                        return Value{static_cast<double>(std::rand()) / RAND_MAX};
                    }
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
                    if (funcName == "lower") {
                        if (call->arguments.size() != 1) throw std::runtime_error("lower() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isString()) throw std::runtime_error("lower() argument must be a string");
                        std::string s = arg.asString();
                        std::transform(s.begin(), s.end(), s.begin(), ::tolower);
                        return Value{s};
                    }
                    if (funcName == "upper") {
                        if (call->arguments.size() != 1) throw std::runtime_error("upper() expects 1 argument");
                        Value arg = evaluate(call->arguments[0].get());
                        if (!arg.isString()) throw std::runtime_error("upper() argument must be a string");
                        std::string s = arg.asString();
                        std::transform(s.begin(), s.end(), s.begin(), ::toupper);
                        return Value{s};
                    }
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
                        if (delimiter.empty()) {
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
                    if (funcName == "read") {
                        if (!call->arguments.empty()) throw std::runtime_error("read() expects 0 arguments");
                        return Value{std::string("")};
                    }
                    if (funcName == "stacktrace") {
                        if (!call->arguments.empty()) throw std::runtime_error("stacktrace() expects 0 arguments");
                        return Value{std::vector<Value>{}};
                    }
                }

                Value funcValue = evaluate(call->callee.get());

                if (!funcValue.isFunction()) {
                    std::string callee_repr = "expression";
                    if(calleeAsIdentifier) callee_repr = calleeAsIdentifier->name;
                    throw std::runtime_error("Attempted to call a non-function value (type: " + funcValue.typeName() + ") derived from: " + callee_repr );
                }

                auto actualFunc = funcValue.asFunction();

                if (call->arguments.size() != actualFunc->parameters.size()) {
                    throw std::runtime_error("Wrong number of arguments for function. Expected " + 
                                             std::to_string(actualFunc->parameters.size()) + ", Got " + std::to_string(call->arguments.size()));
                }

                std::unordered_map<std::string, Value> oldGlobals = globals;
                std::vector<Value> evaluated_args;
                for (const auto& arg_expr : call->arguments) {
                    evaluated_args.push_back(evaluate(arg_expr.get()));
                }
                for (size_t i = 0; i < actualFunc->parameters.size(); ++i) {
                    globals[actualFunc->parameters[i]] = evaluated_args[i];
                }

                try {
                    execute(actualFunc->body.get());
                    globals = oldGlobals;
                    return Value{nullptr}; 
                } catch (const ReturnValue& ret) {
                    globals = oldGlobals;
                    return ret.value;
                }
            }

            if (auto unaryOp = dynamic_cast<UnaryOp*>(expr)) {
                Value operand = evaluate(unaryOp->operand.get());
                if (unaryOp->op == "not") {
                    return Value{static_cast<double>(!operand.asBool())};
                } else if (unaryOp->op == "-") {
                    if (!operand.isNumber()) {
                        throw std::runtime_error("Operand for unary '-' must be a number. Got " + operand.typeName());
                    }
                    return Value{-operand.asNumber()};
                }
                throw std::runtime_error("Unknown unary operator: " + unaryOp->op);
            }

            if (auto indexAccess = dynamic_cast<IndexAccess*>(expr)) {
                Value targetVal = evaluate(indexAccess->target.get());

                if (indexAccess->is_slice_op) {
                    if (targetVal.isString()) {
                        const std::string& str = targetVal.asString();
                        long long start = 0, end = static_cast<long long>(str.length()), step = 1;

                        if (indexAccess->start_slice) {
                            Value startVal = evaluate(indexAccess->start_slice.get());
                            if (!startVal.isNumber()) throw std::runtime_error("Slice start index must be a number.");
                            start = static_cast<long long>(startVal.asNumber());
                        }
                        if (indexAccess->end_slice) {
                            Value endVal = evaluate(indexAccess->end_slice.get());
                            if (!endVal.isNumber()) throw std::runtime_error("Slice end index must be a number.");
                            end = static_cast<long long>(endVal.asNumber());
                        }
                        if (indexAccess->step_slice) {
                            Value stepVal = evaluate(indexAccess->step_slice.get());
                            if (!stepVal.isNumber()) throw std::runtime_error("Slice step must be a number.");
                            step = static_cast<long long>(stepVal.asNumber());
                            if (step == 0) throw std::runtime_error("Slice step cannot be zero.");
                        }

                        long long len = static_cast<long long>(str.length());
                        if (start < 0) start += len;
                        if (end < 0) end += len;
                        start = std::max(0LL, start);
                        end = std::max(0LL, end); // end может быть len
                        start = std::min(len, start);
                        end = std::min(len, end);


                        std::string result_str;
                        if (step > 0) {
                            for (long long i = start; i < end; i += step) {
                                result_str += str[static_cast<size_t>(i)];
                            }
                        } else { // step < 0
                            
                            for (long long i = start; i > end; i += step) { // условие i > end для отрицательного шага
                                if (i < len && i >= 0) { // Проверка границ
                                    result_str += str[static_cast<size_t>(i)];
                                }
                            }
                        }
                        return Value{result_str};
                    } else if (targetVal.isList()) {
                        const auto& list = targetVal.asList();
                        long long start = 0, end = static_cast<long long>(list.size()), step = 1;

                        if (indexAccess->start_slice) {
                            Value startVal = evaluate(indexAccess->start_slice.get());
                            if (!startVal.isNumber()) throw std::runtime_error("Slice start index must be a number.");
                            start = static_cast<long long>(startVal.asNumber());
                        }
                        if (indexAccess->end_slice) {
                            Value endVal = evaluate(indexAccess->end_slice.get());
                            if (!endVal.isNumber()) throw std::runtime_error("Slice end index must be a number.");
                            end = static_cast<long long>(endVal.asNumber());
                        }
                        if (indexAccess->step_slice) {
                            Value stepVal = evaluate(indexAccess->step_slice.get());
                            if (!stepVal.isNumber()) throw std::runtime_error("Slice step must be a number.");
                            step = static_cast<long long>(stepVal.asNumber());
                            if (step == 0) throw std::runtime_error("Slice step cannot be zero.");
                        }
                        
                        long long len = static_cast<long long>(list.size());
                        if (start < 0) start += len;
                        if (end < 0) end += len;
                        start = std::max(0LL, start);
                        end = std::max(0LL, end);
                        start = std::min(len, start);
                        end = std::min(len, end);

                        std::vector<Value> result_list;
                        if (step > 0) {
                            for (long long i = start; i < end; i += step) {
                                result_list.push_back(list[static_cast<size_t>(i)]);
                            }
                        } else { // step < 0
                             for (long long i = start; i > end; i += step) { // условие i > end для отрицательного шага
                                 if (i < len && i >= 0) {
                                    result_list.push_back(list[static_cast<size_t>(i)]);
                                 }
                            }
                        }
                        return Value{result_list};
                    } else {
                        throw std::runtime_error("Slice operation can only be applied to strings or lists.");
                    }
                } else {
                    // Обработка одиночного индекса
                    Value idxVal = evaluate(indexAccess->index.get());
                    if (!idxVal.isNumber()) {
                        throw std::runtime_error("Index must be a number. Got " + idxVal.typeName());
                    }
                double raw_index = idxVal.asNumber();
                if (raw_index != static_cast<long long>(raw_index)) {
                        throw std::runtime_error("Index must be an integer. Got " + std::to_string(raw_index));
                }
                long long int_index = static_cast<long long>(raw_index);

                    if (targetVal.isString()) {
                        const std::string& str = targetVal.asString();
                        long long len = static_cast<long long>(str.length());
                        if (int_index < 0) int_index += len;
                        if (int_index < 0 || int_index >= len) {
                            throw std::runtime_error("String index out of bounds: " + std::to_string(int_index - (int_index >= len ? len : 0)) + ", size: " + std::to_string(len));
                        }
                        return Value{std::string(1, str[static_cast<size_t>(int_index)])};
                    } else if (targetVal.isList()) {
                        const auto& list = targetVal.asList();
                        long long len = static_cast<long long>(list.size());
                         if (int_index < 0) int_index += len;
                        if (int_index < 0 || int_index >= len) {
                            throw std::runtime_error("List index out of bounds: " + std::to_string(int_index - (int_index >= len ? len : 0)) + ", size: " + std::to_string(len));
                }
                return list[static_cast<size_t>(int_index)];
                    } else {
                        throw std::runtime_error("Cannot index non-list/non-string type: " + targetVal.typeName());
                    }
                }
            }

            if (auto assign = dynamic_cast<Assignment*>(expr)) {
                if (!assign->value) {
                    throw std::runtime_error("Null value in assignment to " + assign->name);
                }
                Value val_to_assign = evaluate(assign->value.get());

                if (assign->op == "=") {
                    globals[assign->name] = val_to_assign;
                    return val_to_assign;
                } else {
                    // Составное присваивание
                    auto it = globals.find(assign->name);
                    if (it == globals.end()) {
                        throw std::runtime_error("Undefined variable for compound assignment: " + assign->name);
                    }
                    Value current_val = it->second;
                    Value result_val{nullptr}; // Инициализация по умолчанию

                    // Определяем операцию для BinaryOp
                    std::string binary_op_str = assign->op.substr(0, assign->op.length() - 1); // "+=", "-=" -> "+", "-"
                    if (binary_op_str == "+") {
                        if (current_val.isNumber() && val_to_assign.isNumber()) {
                            result_val = Value{current_val.asNumber() + val_to_assign.asNumber()};
                        } else if (current_val.isString() && val_to_assign.isString()) {
                            result_val = Value{current_val.asString() + val_to_assign.asString()};
                        } else if (current_val.isList() && val_to_assign.isList()) {
                            auto result_list_vec = current_val.asList(); // копия
                            const auto& right_list_vec = val_to_assign.asList();
                            result_list_vec.insert(result_list_vec.end(), right_list_vec.begin(), right_list_vec.end());
                            result_val = Value{result_list_vec};
                        } else {
                            throw std::runtime_error("Operator '" + assign->op + "' cannot be applied to types " + current_val.typeName() + " and " + val_to_assign.typeName());
                        }
                    } else if (binary_op_str == "-") {
                        if (current_val.isNumber() && val_to_assign.isNumber()) {
                            result_val = Value{current_val.asNumber() - val_to_assign.asNumber()};
                        } else if (current_val.isString() && val_to_assign.isString()) {
                            // Вычитание строки (удаление суффикса)
                            const std::string& main_str = current_val.asString();
                            const std::string& suffix_str = val_to_assign.asString();
                            if (main_str.length() >= suffix_str.length() && 
                                main_str.substr(main_str.length() - suffix_str.length()) == suffix_str) {
                                result_val = Value{main_str.substr(0, main_str.length() - suffix_str.length())};
                            } else {
                                result_val = Value{main_str}; // Если не суффикс, возвращаем исходную строку
                            }
                        } else {
                            throw std::runtime_error("Operator '-' cannot be applied to types " + current_val.typeName() + " and " + val_to_assign.typeName());
                        }
                    } else if (binary_op_str == "*") {
                         if (current_val.isNumber() && val_to_assign.isNumber()) {
                            result_val = Value{current_val.asNumber() * val_to_assign.asNumber()};
                        } else if ((current_val.isString() && val_to_assign.isNumber()) || (current_val.isNumber() && val_to_assign.isString())) {
                            const std::string* str_val_ptr;
                            double num_val;
                            if (current_val.isString()) { str_val_ptr = &current_val.asString(); num_val = val_to_assign.asNumber(); }
                            else { str_val_ptr = &val_to_assign.asString(); num_val = current_val.asNumber(); } // Это неверно для var *= num
                            if (current_val.isString() && val_to_assign.isNumber()) {
                                str_val_ptr = &current_val.asString(); num_val = val_to_assign.asNumber();
                            } else if (current_val.isNumber() && val_to_assign.isString()) { // num *= str не поддерживается
                                 throw std::runtime_error("Operator '*=' with number on left and string on right is not supported.");
                            } else {
                                throw std::runtime_error("Internal error or invalid types for '*='");
                            }

                            if (num_val < 0) throw std::runtime_error("Cannot multiply string by negative number for '*='");
                            std::string temp_result_str;
                            for (int i = 0; i < static_cast<int>(num_val); ++i) { temp_result_str += *str_val_ptr; }
                            result_val = Value{temp_result_str};

                        } else if ((current_val.isList() && val_to_assign.isNumber())) {
                            const auto& list_val_vec = current_val.asList();
                            double num_val = val_to_assign.asNumber();
                            if (num_val < 0) throw std::runtime_error("Cannot multiply list by negative number for '*='");
                            std::vector<Value> temp_result_list;
                            for (int i = 0; i < static_cast<int>(num_val); ++i) { 
                                temp_result_list.insert(temp_result_list.end(), list_val_vec.begin(), list_val_vec.end()); 
                            }
                            result_val = Value{temp_result_list};
                        } else {
                            throw std::runtime_error("Operator '" + assign->op + "' cannot be applied to types " + current_val.typeName() + " and " + val_to_assign.typeName());
                        }
                    } else if (binary_op_str == "/") {
                        if (current_val.isNumber() && val_to_assign.isNumber()) {
                            if (val_to_assign.asNumber() == 0) throw std::runtime_error("Division by zero for '/='");
                            result_val = Value{current_val.asNumber() / val_to_assign.asNumber()};
                        } else {
                            throw std::runtime_error("Operator '" + assign->op + "' requires numeric operands");
                        }
                    } else if (binary_op_str == "%") {
                        if (current_val.isNumber() && val_to_assign.isNumber()) {
                            if (val_to_assign.asNumber() == 0) throw std::runtime_error("Modulo by zero for '%='");
                            result_val = Value{std::fmod(current_val.asNumber(), val_to_assign.asNumber())};
                        } else {
                            throw std::runtime_error("Operator '" + assign->op + "' requires numeric operands");
                        }
                    } else if (binary_op_str == "^") {
                         if (current_val.isNumber() && val_to_assign.isNumber()) {
                            result_val = Value{std::pow(current_val.asNumber(), val_to_assign.asNumber())};
                        } else {
                            throw std::runtime_error("Operator '" + assign->op + "' requires numeric operands");
                        }
                    } else {
                        throw std::runtime_error("Unsupported compound assignment operator: " + assign->op);
                    }
                    
                    globals[assign->name] = result_val;
                    return result_val;
                }
            }

            if (auto id = dynamic_cast<Identifier*>(expr)) {
                if (id->name == "print" || id->name == "range" || id->name == "len" ||
                    id->name == "push" || id->name == "pop" || id->name == "insert" ||
                    id->name == "remove" || id->name == "sort" || id->name == "abs" ||
                    id->name == "ceil" || id->name == "floor" || id->name == "round" ||
                    id->name == "sqrt" || id->name == "rnd" || id->name == "parse_num" ||
                    id->name == "to_string" || id->name == "lower" || id->name == "upper" ||
                    id->name == "split" || id->name == "join" || id->name == "replace" ||
                    id->name == "println" || id->name == "read" || id->name == "stacktrace"
                ) {
                    throw std::runtime_error("Built-in function '" + id->name + "' must be called with parentheses ()");
                }
                
                auto it = globals.find(id->name);
                if (it == globals.end()) {
                    throw std::runtime_error("Undefined variable: " + id->name);
                }
                return it->second;
            }

            if (auto binOp = dynamic_cast<BinaryOp*>(expr)) {
                auto left = evaluate(binOp->left.get());
                auto right = evaluate(binOp->right.get());

                if (binOp->op == "+") {
                    if (left.isNil() || right.isNil()) {
                        throw std::runtime_error("Operator '+' cannot be applied if an operand is Nil");
                    }
                    if (left.isNumber() && right.isNumber()) {
                        return Value{left.asNumber() + right.asNumber()};
                    } else if (left.isString() && right.isString()) {
                        return Value{left.asString() + right.asString()};
                    } else if (left.isList() && right.isList()) {
                        auto result_list = left.asList();
                        const auto& right_list = right.asList();
                        result_list.insert(result_list.end(), right_list.begin(), right_list.end());
                        return Value{result_list};
                    } else {
                        throw std::runtime_error("Operator '+' cannot be applied to types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "-") {
                    if (left.isNil() || right.isNil()) {
                        throw std::runtime_error("Operator '-' cannot be applied if an operand is Nil");
                    }
                    if (left.isNumber() && right.isNumber()) {
                        return Value{left.asNumber() - right.asNumber()};
                    } else if (left.isString() && right.isString()) {
                        // Вычитание строки (удаление суффикса)
                        const std::string& main_str = left.asString();
                        const std::string& suffix_str = right.asString();
                        if (main_str.length() >= suffix_str.length() && 
                            main_str.substr(main_str.length() - suffix_str.length()) == suffix_str) {
                            return Value{main_str.substr(0, main_str.length() - suffix_str.length())};
                        } else {
                            return Value{main_str}; // Если не суффикс, возвращаем исходную строку
                        }
                    } else {
                        throw std::runtime_error("Operator '-' cannot be applied to types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "/" || binOp->op == "%" || binOp->op == "^") {
                    if (left.isNil() || right.isNil()) {
                        throw std::runtime_error("Operator '" + binOp->op + "' cannot be applied if an operand is Nil");
                    }
                    if (left.isNumber() && right.isNumber()) {
                        double num_left = left.asNumber();
                        double num_right = right.asNumber(); 
                        if ((binOp->op == "/" || binOp->op == "%") && num_right == 0.0) {
                            throw std::runtime_error(binOp->op == "/" ? "Division by zero" : "Modulo by zero");
                        }
                        if (binOp->op == "-") return Value{num_left - num_right};
                        if (binOp->op == "/") return Value{num_left / num_right};
                        if (binOp->op == "%") return Value{std::fmod(num_left, num_right)};
                        if (binOp->op == "^") return Value{std::pow(num_left, num_right)};
                    } else {
                        throw std::runtime_error("Operator '" + binOp->op + "' cannot be applied to types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "*") {
                    if (left.isNumber() && right.isNumber()) {
                        return Value{left.asNumber() * right.asNumber()};
                    } else if ((left.isString() && right.isNumber()) || (left.isNumber() && right.isString())) {
                        const std::string* str_val;
                        double num_val;
                        if (left.isString()) { str_val = &left.asString(); num_val = right.asNumber(); }
                        else { str_val = &right.asString(); num_val = left.asNumber(); }
                        if (num_val < 0) throw std::runtime_error("Cannot multiply string by negative number");
                        std::string result_str;
                        for (int i = 0; i < static_cast<int>(num_val); ++i) { result_str += *str_val; }
                        return Value{result_str};
                    } else if ((left.isList() && right.isNumber()) || (left.isNumber() && right.isList())) {
                        const std::vector<Value>* list_val;
                        double num_val;
                        if (left.isList()) { list_val = &left.asList(); num_val = right.asNumber(); }
                        else { list_val = &right.asList(); num_val = left.asNumber(); }
                        if (num_val < 0) throw std::runtime_error("Cannot multiply list by negative number");
                        std::vector<Value> result_list;
                        for (int i = 0; i < static_cast<int>(num_val); ++i) { result_list.insert(result_list.end(), list_val->begin(), list_val->end()); }
                        return Value{result_list};
                    } else {
                        throw std::runtime_error("Operator '*' cannot be applied to types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "==" || binOp->op == "!=") {
                    if (left.isNil() && right.isNil()) return Value{static_cast<double>(binOp->op == "==")};
                    if (left.isNil() || right.isNil()) return Value{static_cast<double>(binOp->op == "!=")};
                    if (left.isNumber() && right.isNumber()) {
                        if (binOp->op == "==") return Value{static_cast<double>(left.asNumber() == right.asNumber())};
                        if (binOp->op == "!=") return Value{static_cast<double>(left.asNumber() != right.asNumber())};
                    } else if (left.isString() && right.isString()) {
                        if (binOp->op == "==") return Value{static_cast<double>(left.asString() == right.asString())};
                        if (binOp->op == "!=") return Value{static_cast<double>(left.asString() != right.asString())};
                    } else {
                        throw std::runtime_error("Operator '" + binOp->op + "' cannot compare types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "<" || binOp->op == ">" || binOp->op == "<=" || binOp->op == ">=") {
                    if (left.isNil() || right.isNil()) {
                        throw std::runtime_error("Operator '" + binOp->op + "' cannot be applied if an operand is Nil");
                    }
                    if (left.isNumber() && right.isNumber()) {
                        double l = left.asNumber(), r = right.asNumber();
                        if (binOp->op == "<") return Value{static_cast<double>(l < r)};
                        if (binOp->op == ">") return Value{static_cast<double>(l > r)};
                        if (binOp->op == "<=") return Value{static_cast<double>(l <= r)};
                        if (binOp->op == ">=") return Value{static_cast<double>(l >= r)};
                    } else if (left.isString() && right.isString()) {
                        const std::string& l = left.asString(), &r = right.asString();
                        if (binOp->op == "<") return Value{static_cast<double>(l < r)};
                        if (binOp->op == ">") return Value{static_cast<double>(l > r)};
                        if (binOp->op == "<=") return Value{static_cast<double>(l <= r)};
                        if (binOp->op == ">=") return Value{static_cast<double>(l >= r)};
                    } else {
                        throw std::runtime_error("Operator '" + binOp->op + "' cannot be applied to types " + left.typeName() + " and " + right.typeName());
                    }
                } else if (binOp->op == "and" || binOp->op == "or") {
                    if (binOp->op == "and") return Value{static_cast<double>(left.asBool() && right.asBool())};
                    if (binOp->op == "or") return Value{static_cast<double>(left.asBool() || right.asBool())};
                }
                throw std::runtime_error("Unknown operator '" + binOp->op + "' for types " + left.typeName() + " and " + right.typeName());
            }

            throw std::runtime_error("Unknown expression type in evaluate: " + std::string(typeid(*expr).name()));
        }

    Value execute(ASTNode* node) {
        if (auto returnStmt = dynamic_cast<ReturnStatement*>(node)) {
            if (returnStmt->value) {
                throw ReturnValue(evaluate(returnStmt->value.get()));
            } else {
                throw ReturnValue(Value{nullptr}); 
            }
        }
        if (auto breakStmt = dynamic_cast<BreakStatement*>(node)) {
            throw BreakSignal();
        }
        if (auto continueStmt = dynamic_cast<ContinueStatement*>(node)) {
            throw ContinueSignal();
        }
        if (auto exprStmt = dynamic_cast<ExpressionStatement*>(node)) {
            return evaluate(exprStmt->expression.get());
        }
        if (auto expr = dynamic_cast<Expression*>(node)) {
            return evaluate(expr);
        }
        if (auto block = dynamic_cast<Block*>(node)) {
            Value lastValue{nullptr};
            for (const auto& stmt : block->statements) {
                lastValue = execute(stmt.get());
            }
            return lastValue;
        }
        if (auto ifStmt = dynamic_cast<IfStatement*>(node)) {
            Value condValue = evaluate(ifStmt->condition.get());
            
            if (condValue.asBool()) {
                return execute(ifStmt->thenBlock.get());
            } else {
                for (auto& elseIf : ifStmt->elseIfBlocks) {
                    Value elseIfCondValue = evaluate(elseIf.first.get());
                    if (elseIfCondValue.asBool()) {
                        return execute(elseIf.second.get());
                    }
                }
                if (ifStmt->elseBlock) {
                    return execute(ifStmt->elseBlock.get());
                }
            }
            return Value{nullptr};
        }
        if (auto whileStmt = dynamic_cast<WhileStatement*>(node)) {
            while (evaluate(whileStmt->condition.get()).asBool()) {
                try {
                    execute(whileStmt->body.get());
                } catch (const BreakSignal&) {
                    break; 
                } catch (const ContinueSignal&) {
                    continue; 
                }
            }
            return Value{nullptr};
        }
        if (auto forStmt = dynamic_cast<ForStatement*>(node)) {
            if (auto call = dynamic_cast<FunctionCall*>(forStmt->iterable.get())) {
                if (auto id = dynamic_cast<Identifier*>(call->callee.get())) {
                    if (id->name == "range") {
                        Value rangeValue = evaluate(call);
                        const auto& list = rangeValue.asList(); 
                        for (const auto& item : list) {
                            globals[forStmt->variableName] = item;
                            try {
                                execute(forStmt->body.get());
                            } catch (const BreakSignal&) {
                                goto end_for_loop; 
                            } catch (const ContinueSignal&) {
                            }
                        }
                        end_for_loop:;
                        return Value{nullptr};
                    }
                }
            }
            
            Value iterableValue = evaluate(forStmt->iterable.get());

            if (iterableValue.isList()) {
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
            } else if (iterableValue.isString()) {
                const std::string& str = iterableValue.asString();
                for (char ch : str) {
                    globals[forStmt->variableName] = Value{std::string(1, ch)};
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
            return Value{nullptr};
        }
        throw std::runtime_error("Unknown ASTNode type: " + std::string(typeid(*node).name()));
        return Value{nullptr};
    }
};


    