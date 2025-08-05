#pragma once

#include <memory>
#include <vector>
#include <variant>
#include <stdexcept>

#include "lexer.h"

// базовый класс для всех узлов абстрактного синтаксического дерева
struct ASTNode {
    virtual ~ASTNode() = default;
};

// базовый класс для всех выражений
struct Expression : ASTNode {
    virtual Expression* clone() const = 0;
};

// числовой литерал
struct NumberLiteral : Expression {
    double value;
    NumberLiteral(double v) : value(v) {}
    Expression* clone() const override {
        return new NumberLiteral(value);
    }
};

// строковый литерал
struct StringLiteral : Expression {
    std::string value;
    StringLiteral(const std::string& v) : value(v) {}
    Expression* clone() const override {
        return new StringLiteral(value);
    }
};

// значение nil
struct NilLiteral : Expression {
    NilLiteral() {}
    Expression* clone() const override {
        return new NilLiteral();
    }
};

// список значений
struct ListLiteral : Expression {
    std::vector<std::unique_ptr<Expression>> elements;
    ListLiteral(std::vector<std::unique_ptr<Expression>> elems)
        : elements(std::move(elems)) {}
    Expression* clone() const override {
        std::vector<std::unique_ptr<Expression>> clonedElements;
        for (const auto& elem : elements) {
            clonedElements.push_back(std::unique_ptr<Expression>(elem->clone()));
        }
        return new ListLiteral(std::move(clonedElements));
    }
};

// идентификатор переменной
struct Identifier : Expression {
    std::string name;
    Identifier(const std::string& n) : name(n) {}
    Expression* clone() const override {
        return new Identifier(name);
    }
};

// бинарная операция
struct BinaryOp : Expression {
    std::string op;
    std::unique_ptr<Expression> left;
    std::unique_ptr<Expression> right;
    BinaryOp(std::string o, std::unique_ptr<Expression> l, std::unique_ptr<Expression> r)
        : op(o), left(std::move(l)), right(std::move(r)) {}
    Expression* clone() const override {
        return new BinaryOp(op,
            std::unique_ptr<Expression>(left->clone()),
            std::unique_ptr<Expression>(right->clone()));
    }
};

// вызов функции
struct FunctionCall : Expression {
    std::unique_ptr<Expression> callee;
    std::vector<std::unique_ptr<Expression>> arguments;
    FunctionCall(std::unique_ptr<Expression> c, std::vector<std::unique_ptr<Expression>> args)
        : callee(std::move(c)), arguments(std::move(args)) {}
    Expression* clone() const override {
        std::vector<std::unique_ptr<Expression>> clonedArgs;
        for (const auto& arg : arguments) {
            clonedArgs.push_back(std::unique_ptr<Expression>(arg->clone()));
        }
        return new FunctionCall(
            std::unique_ptr<Expression>(callee->clone()),
            std::move(clonedArgs));
    }
};

// базовый класс для всех инструкций
struct Statement : ASTNode {
    virtual Statement* clone() const = 0;
};

// инструкция-выражение
struct ExpressionStatement : Statement {
    std::unique_ptr<Expression> expression;
    
    explicit ExpressionStatement(std::unique_ptr<Expression> expr) 
        : expression(std::move(expr)) {}
    Statement* clone() const override {
        return new ExpressionStatement(std::unique_ptr<Expression>(expression->clone()));
    }
};

// инструкция return
struct ReturnStatement : Statement {
    std::unique_ptr<Expression> value;

    explicit ReturnStatement(std::unique_ptr<Expression> v)
        : value(std::move(v)) {}
    Statement* clone() const override {
        if (value) {
            return new ReturnStatement(std::unique_ptr<Expression>(value->clone()));
        }
        return new ReturnStatement(nullptr);
    }
};

// блок кода
struct Block : ASTNode {
    std::vector<std::unique_ptr<ASTNode>> statements;

    Block* clone() const {
        auto newBlock = new Block();
        for (const auto& stmtNode : statements) {
            if (auto stmt = dynamic_cast<Statement*>(stmtNode.get())) {
                 newBlock->statements.push_back(std::unique_ptr<Statement>(stmt->clone()));
            } 
        }
        return newBlock;
    }
};

// определение функции
struct FunctionDefinition : Expression {
    std::vector<std::string> parameters;
    std::unique_ptr<Block> body;

    FunctionDefinition(std::vector<std::string> params, std::unique_ptr<Block> b)
        : parameters(std::move(params)), body(std::move(b)) {}
    Expression* clone() const override {
        return new FunctionDefinition(
            parameters,
            std::unique_ptr<Block>(body->clone()));
    }
};

// присваивание значения переменной
struct Assignment : Expression {
    std::string name;
    std::unique_ptr<Expression> value;
    std::string op; // "=", "+=", "-=", etc.

    Assignment(std::string n, std::unique_ptr<Expression> v, std::string o = "=")
        : name(std::move(n)), value(std::move(v)), op(std::move(o)) {}
    Expression* clone() const override {
        return new Assignment(name, std::unique_ptr<Expression>(value->clone()), op);
    }
};

// условный оператор if
struct IfStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Block> thenBlock;
    std::unique_ptr<Block> elseBlock;
    std::vector<std::pair<std::unique_ptr<Expression>, std::unique_ptr<Block>>> elseIfBlocks;

    IfStatement(std::unique_ptr<Expression> cond, std::unique_ptr<Block> thenBlk)
        : condition(std::move(cond)), thenBlock(std::move(thenBlk)) {}
    Statement* clone() const override {
        auto ifStmt = std::make_unique<IfStatement>(
            std::unique_ptr<Expression>(condition->clone()),
            std::unique_ptr<Block>(thenBlock->clone())
        );

        for (auto& [cond, block] : elseIfBlocks) {
            ifStmt->elseIfBlocks.emplace_back(
                std::unique_ptr<Expression>(cond->clone()),
                std::unique_ptr<Block>(block->clone())
            );
        }

        if (elseBlock) {
            ifStmt->elseBlock = std::unique_ptr<Block>(elseBlock->clone());
        }

        return ifStmt.release();
    }
};

// цикл while
struct WhileStatement : Statement {
    std::unique_ptr<Expression> condition;
    std::unique_ptr<Block> body;

    WhileStatement(std::unique_ptr<Expression> cond, std::unique_ptr<Block> bdy)
        : condition(std::move(cond)), body(std::move(bdy)) {}
    Statement* clone() const override {
        return new WhileStatement(
            std::unique_ptr<Expression>(condition->clone()),
            std::unique_ptr<Block>(body->clone())
        );
    }
};

// цикл for
struct ForStatement : Statement {
    std::string variableName;
    std::unique_ptr<Expression> iterable;
    std::unique_ptr<Block> body;

    ForStatement(std::string varName, std::unique_ptr<Expression> iter, std::unique_ptr<Block> bdy)
        : variableName(std::move(varName)), iterable(std::move(iter)), body(std::move(bdy)) {}
    Statement* clone() const override {
        return new ForStatement(
            variableName,
            std::unique_ptr<Expression>(iterable->clone()),
            std::unique_ptr<Block>(body->clone())
        );
    }
};

// унарная операция
struct UnaryOp : Expression {
    std::string op;
    std::unique_ptr<Expression> operand;
    UnaryOp(std::string o, std::unique_ptr<Expression> e) : op(o), operand(std::move(e)) {}
    Expression* clone() const override {
        return new UnaryOp(op, std::unique_ptr<Expression>(operand->clone()));
    }
};

// доступ к элементу по индексу
struct IndexAccess : Expression {
    std::unique_ptr<Expression> target;
    std::unique_ptr<Expression> index;
    std::unique_ptr<Expression> start_slice;
    std::unique_ptr<Expression> end_slice;
    std::unique_ptr<Expression> step_slice;
    bool is_slice_op = false;

    IndexAccess(std::unique_ptr<Expression> t, std::unique_ptr<Expression> i)
        : target(std::move(t)), index(std::move(i)), is_slice_op(false) {}

    IndexAccess(std::unique_ptr<Expression> t,
                std::unique_ptr<Expression> s,
                std::unique_ptr<Expression> e,
                std::unique_ptr<Expression> st)
        : target(std::move(t)), start_slice(std::move(s)),
          end_slice(std::move(e)), step_slice(std::move(st)), is_slice_op(true) {}

    Expression* clone() const override {
        if (is_slice_op) {
            return new IndexAccess(
                std::unique_ptr<Expression>(target->clone()),
                start_slice ? std::unique_ptr<Expression>(start_slice->clone()) : nullptr,
                end_slice ? std::unique_ptr<Expression>(end_slice->clone()) : nullptr,
                step_slice ? std::unique_ptr<Expression>(step_slice->clone()) : nullptr
            );
        } else {
            if (!index) throw std::runtime_error("Attempting to clone IndexAccess with null index");
            return new IndexAccess(
                std::unique_ptr<Expression>(target->clone()),
                std::unique_ptr<Expression>(index->clone())
            );
        }
    }
};

// оператор break
struct BreakStatement : Statement {
    BreakStatement() = default;
    Statement* clone() const override { return new BreakStatement(); }
};

// оператор continue
struct ContinueStatement : Statement {
    ContinueStatement() = default;
    Statement* clone() const override { return new ContinueStatement(); }
};

// синтаксический анализатор
class Parser {
    std::vector<Token> tokens;
    size_t current = 0;
    
    const Token& peek() const { return tokens[current]; }
    bool isAtEnd() const { return peek().type == TokenType::EndOfFile; }
    const Token& previous() const { return tokens[current - 1]; }
    
    bool match(TokenType type) {
        if (check(type)) {
            advance();
            return true;
        }
        return false;
    }
    
    bool check(TokenType type) const {
        if (isAtEnd()) return false;
        return peek().type == type;
    }
    
    const Token& advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    std::unique_ptr<Expression> parseTerm() {
        auto expr = parseFactor();

        while (match(TokenType::Operator)) {
            std::string op = previous().value;
            
            if (op == "+" || op == "-") {
                auto right = parseFactor();
                expr = std::make_unique<BinaryOp>(op, std::move(expr), std::move(right));
            } else {
                current--;
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<Expression> parseComparison() {
        auto expr = parseTerm();
        
        while (peek().type == TokenType::Operator) {
            std::string op = peek().value;
            if (op == "<" || op == "<=" || op == ">" || op == ">=") {
                advance(); 
                auto right = parseTerm();
                expr = std::make_unique<BinaryOp>(op, std::move(expr), std::move(right));
            } else {
                break;
            }
        }
        
        return expr;
    }

    std::unique_ptr<Expression> parseUnary() {
        if (peek().type == TokenType::Keyword && peek().value == "not") {
            advance(); // Consume "not"
            auto operand = parseUnary(); 
            if (!operand) throw std::runtime_error("Expected operand after 'not'");
            return std::make_unique<UnaryOp>("not", std::move(operand));
        } else if (peek().type == TokenType::Operator && peek().value == "-") {
            advance(); // Consume "-"
            auto operand = parseUnary();
            if (!operand) throw std::runtime_error("Expected operand after unary '-'");
            return std::make_unique<UnaryOp>("-", std::move(operand));
        }
        return parsePrimary();
    }

    std::unique_ptr<Expression> parseFactor() {
        auto expr = parseUnary();

        while (match(TokenType::Operator)) {
            std::string op = previous().value;
            
            if (op == "*" || op == "/" || op == "%") {
                auto right = parseUnary();
                expr = std::make_unique<BinaryOp>(op, std::move(expr), std::move(right));
            } else {
                current--;
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<Expression> parsePrimary() {
        if (isAtEnd()) {
            throw std::runtime_error("Parser error: Unexpected end of input while expecting a primary expression.");
        }

        std::unique_ptr<Expression> expr;

        if (peek().type == TokenType::Identifier) {
            advance(); 
            expr = std::make_unique<Identifier>(previous().value);
        } else if (peek().type == TokenType::Number) {
            advance(); 
            expr = std::make_unique<NumberLiteral>(std::stod(previous().value));
        } else if (peek().type == TokenType::String) {
            advance(); 
            expr = std::make_unique<StringLiteral>(previous().value);
        } else if (peek().type == TokenType::LParen) {
            advance(); 
            expr = parseExpression();
            if (!match(TokenType::RParen)) {
                throw std::runtime_error("Expected ')' after expression in parentheses");
            }
        } else if (peek().type == TokenType::LBracket) {
            advance();
            std::vector<std::unique_ptr<Expression>> elements;
            if (!check(TokenType::RBracket)) {
                do {
                    auto element = parseExpression();
                    if (!element) {
                        // Эта ошибка может возникнуть, если, например, список заканчивается запятой: [1,]
                        // Или если элемент невалиден: [1, if]
                        throw std::runtime_error("Invalid list element or trailing comma");
                    }
                    elements.push_back(std::move(element));
                } while (match(TokenType::Comma));
            }
            if (!match(TokenType::RBracket)) {
                throw std::runtime_error("Expected ']' after list elements");
            }
            expr = std::make_unique<ListLiteral>(std::move(elements));
        } else if (peek().type == TokenType::Keyword) {
            std::string keyword = advance().value;
            if (keyword == "true") expr = std::make_unique<NumberLiteral>(1.0);
            else if (keyword == "false") expr = std::make_unique<NumberLiteral>(0.0);
            else if (keyword == "nil") expr = std::make_unique<NilLiteral>();
            else if (keyword == "function") {
                consume(TokenType::LParen, "Expected '(' after 'function'");
                std::vector<std::string> parameters;
                if (!check(TokenType::RParen)) {
                    do {
                        if (!check(TokenType::Identifier)) {
                            throw std::runtime_error("Expected parameter name, got: " + peek().value);
                        }
                        parameters.push_back(advance().value);
                    } while (match(TokenType::Comma));
                }
                consume(TokenType::RParen, "Expect ')' after function parameters");
                
                auto body = parseBlock();
                
                consume(TokenType::Keyword, "Expected 'end' after function body");
                if (previous().value != "end") { // Убедимся, что это именно "end"
                    throw std::runtime_error("Expected keyword 'end' but got '" + previous().value + "' after function body");
                }

                consume(TokenType::Keyword, "Expected 'function' after 'end' for function definition");
                if (previous().value != "function") { // Убедимся, что это именно "function"
                     throw std::runtime_error("Expected keyword 'function' after 'end' but got '" + previous().value + "'");
                }
                                
                expr = std::make_unique<FunctionDefinition>(std::move(parameters), std::move(body));
            } else {
                current--; 
                throw std::runtime_error("Unexpected keyword in primary expression: " + keyword);
            }
        } else {
            throw std::runtime_error("Unexpected token in primary expression: " + peek().value);
        }

        while (true) {
            if (peek().type == TokenType::LParen) { 
                advance(); 
                std::vector<std::unique_ptr<Expression>> args;
                if (!check(TokenType::RParen)) {
                    do {
                        auto arg = parseExpression();
                        if (!arg) {
                            throw std::runtime_error("Invalid function argument");
                        }
                        args.push_back(std::move(arg));
                    } while (match(TokenType::Comma));
                }
                if (!match(TokenType::RParen)) {
                    std::string calleeName = "expression";
                    if (auto id = dynamic_cast<Identifier*>(expr.get())) {
                        calleeName = id->name;
                    }
                    throw std::runtime_error("Expected ')' after function arguments for " + calleeName);
                }
                expr = std::make_unique<FunctionCall>(std::move(expr), std::move(args));
            } else if (peek().type == TokenType::LBracket) { // Теперь это только для IndexAccess/Slice
                // Убедимся, что expr (то, что индексируется/срезается) уже было разобрано
                if (!expr) {
                    throw std::runtime_error("Internal parser error: LBracket encountered for indexing without a preceding expression.");
                }
                advance(); 
                std::unique_ptr<Expression> p_index, p_start, p_end, p_step;
                bool is_slice_syntax = false;

                if (check(TokenType::Colon)) {
                    is_slice_syntax = true;
                } else if (!check(TokenType::RBracket)) {
                    p_start = parseExpression();
                }

                if (match(TokenType::Colon)) {
                    is_slice_syntax = true;

                    if (!check(TokenType::RBracket) && !check(TokenType::Colon)) {
                        p_end = parseExpression();
                    }

                    if (match(TokenType::Colon)) {
                        if (!check(TokenType::RBracket)) {
                            p_step = parseExpression();
                        }
                    }
                }

                consume(TokenType::RBracket, "Expected ']' after index or slice expression");

                if (is_slice_syntax) {
                    expr = std::make_unique<IndexAccess>(std::move(expr), std::move(p_start), std::move(p_end), std::move(p_step));
                } else {
                    if (!p_start) {
                        throw std::runtime_error("Invalid empty index operation on an expression. List literals are `[]`, index is `expr[index]`.");
                    }
                    expr = std::make_unique<IndexAccess>(std::move(expr), std::move(p_start));
                }
            } else {
                break;
            }
        }
        return expr;
    }

    std::unique_ptr<IfStatement> parseIfStatement() {
        advance(); 

        auto condition = parseExpression();

        if (!match(TokenType::Keyword) || previous().value != "then") {
            throw std::runtime_error("Expected 'then' after if condition");
        }

        auto thenBlock = parseBlock();

        auto ifStmt = std::make_unique<IfStatement>(std::move(condition), std::move(thenBlock));

        while (check(TokenType::Keyword) && peek().value == "else") {
            advance();

            if (check(TokenType::Keyword) && peek().value == "if") {
                advance();
                auto elseIfCondition = parseExpression();
                if (!match(TokenType::Keyword) || previous().value != "then") {
                    throw std::runtime_error("Expected 'then' after else if condition");
                }
                auto elseIfBlock = parseBlock();
                ifStmt->elseIfBlocks.emplace_back(std::move(elseIfCondition), std::move(elseIfBlock));
            } else {
                ifStmt->elseBlock = parseBlock();
                break;
            }
        }

        if (!match(TokenType::Keyword) || previous().value != "end") {
            throw std::runtime_error("Expected 'end' after if/else if/else blocks. Got: " + (isAtEnd() ? "EOF" : previous().value));
        }
        if (!match(TokenType::Keyword) || previous().value != "if") {
            throw std::runtime_error("Expected 'if' after 'end' for IfStatement. Got: " + (isAtEnd() ? "EOF" : previous().value));
        }

        return ifStmt;
    }

    std::unique_ptr<Expression> parseExpression() {
        return parseAssignment();
    }

    std::unique_ptr<Expression> parseAssignment() {
        auto expr = parseLogicalOr(); // Сначала парсим левую часть как выражение высшего приоритета

        // Проверяем, является ли следующий токен оператором присваивания
        if (peek().type == TokenType::Operator) {
            std::string op = peek().value;
            if (op == "=" || op == "+=" || op == "-=" || op == "*=" || 
                op == "/=" || op == "%=" || op == "^=") {
                
                // Убедимся, что левая часть (expr) является валидным lvalue (идентификатором)
                Identifier* lvalue = dynamic_cast<Identifier*>(expr.get());
                if (!lvalue) {
                    throw std::runtime_error("Invalid target for assignment. Expected an identifier.");
                }
                
                advance(); // Съедаем оператор присваивания
                
                auto valueExpr = parseAssignment(); 
                if (!valueExpr) {
                    throw std::runtime_error("Expected expression after assignment operator '" + op + "'");
                }
                return std::make_unique<Assignment>(lvalue->name, std::move(valueExpr), op);
            }
        }
        
        return expr; 
    }

    std::unique_ptr<Expression> parseLogicalOr() {
        auto expr = parseLogicalAnd();
        
        while (peek().type == TokenType::Keyword && peek().value == "or") {
            advance(); 
            auto right = parseLogicalAnd();
            expr = std::make_unique<BinaryOp>("or", std::move(expr), std::move(right));
        }
        
        return expr;
    }

    std::unique_ptr<Expression> parseLogicalAnd() {
        auto expr = parseEquality();
        
        while (peek().type == TokenType::Keyword && peek().value == "and") {
            advance(); 
            auto right = parseEquality();
            expr = std::make_unique<BinaryOp>("and", std::move(expr), std::move(right));
        }
        
        return expr;
    }

    std::unique_ptr<Expression> parseEquality() {
        auto expr = parseComparison();
        
        while (peek().type == TokenType::Operator) {
            std::string op = peek().value;
            if (op == "==" || op == "!=") {
                advance(); 
                auto right = parseComparison();
                expr = std::make_unique<BinaryOp>(op, std::move(expr), std::move(right));
            } else {
                break;
            }
        }
        
        return expr;
    }

public:
    Parser(std::vector<Token> tks) : tokens(std::move(tks)) {}
    
    std::unique_ptr<Block> parse() {
    auto block = std::make_unique<Block>();
    while (!isAtEnd()) {
        while (match(TokenType::EndOfLine)) {
        }

        if (!isAtEnd()) {
            block->statements.push_back(parseStatement());
        }
    }
    return block;
}

    
private:
    std::unique_ptr<ASTNode> parseStatement() {
        if (peek().type == TokenType::Keyword) {
            const std::string& keyword_val = peek().value;
            if (keyword_val == "if") {
                return parseIfStatement();
            }
            if (keyword_val == "while") {
                return parseWhileStatement();
            }
            if (keyword_val == "for") {
                return parseForStatement();
            }
            if (keyword_val == "return") {
                advance(); 
                std::unique_ptr<Expression> value = nullptr;
                if (peek().type != TokenType::EndOfLine && 
                    !(peek().type == TokenType::Keyword && peek().value == "end")) {
                    value = parseExpression();
                }
                return std::make_unique<ReturnStatement>(std::move(value));
            }
            if (keyword_val == "break") {
                advance(); 
                return std::make_unique<BreakStatement>();
            }
            if (keyword_val == "continue") {
                advance(); 
                return std::make_unique<ContinueStatement>();
            }
        }
        // Если это не ключевое слово, то это должно быть выражение
        auto expr = parseExpression(); 
        // Проверяем наличие точки с запятой или конца строки
        if (peek().type == TokenType::Semicolon) {
            advance(); 
        } else if (peek().type == TokenType::EndOfLine) {
            // Ничего не делаем, просто пропускаем
        } else if (isAtEnd()) {
            // Ничего не делаем, конец файла
        } 

        return std::make_unique<ExpressionStatement>(std::move(expr));
    }
    
    std::unique_ptr<Block> parseBlock() {
        auto block = std::make_unique<Block>();
        
        while (peek().type == TokenType::EndOfLine) {
            advance();
        }

        while (!isAtEnd() && 
               !(peek().type == TokenType::Keyword && 
                 (peek().value == "end" || peek().value == "else" || peek().value == "else if"))) {
            block->statements.push_back(parseStatement());
            
            while (peek().type == TokenType::EndOfLine) {
                advance();
            }
        }
        return block;
    }
    
    const Token& consume(TokenType type, const std::string& message) {
        if (check(type)) return advance();
        throw std::runtime_error(message);
    }

    std::unique_ptr<WhileStatement> parseWhileStatement() {
        advance(); 
        auto condition = parseExpression();
        auto body = parseBlock();

        if (!match(TokenType::Keyword) || previous().value != "end") {
            throw std::runtime_error("Expected 'end' after while body. Got: " + (isAtEnd() ? "EOF" : previous().value));
        }
        if (!match(TokenType::Keyword) || previous().value != "while") {
            throw std::runtime_error("Expected 'while' after 'end' for while statement. Got: " + (isAtEnd() ? "EOF" : previous().value));
        }
        return std::make_unique<WhileStatement>(std::move(condition), std::move(body));
    }

    std::unique_ptr<ForStatement> parseForStatement() {
        advance(); 
        std::string varName = consume(TokenType::Identifier, "Expected identifier after 'for'.").value;

        if (!match(TokenType::Keyword) || previous().value != "in") {
            throw std::runtime_error("Expected 'in' after for variable. Got: " + (isAtEnd() ? "EOF" : previous().value));
        }
        auto iterable = parseExpression();
        auto body = parseBlock();

        if (!match(TokenType::Keyword) || previous().value != "end") {
            throw std::runtime_error("Expected 'end' after for body. Got: " + (isAtEnd() ? "EOF" : previous().value));
        }
        if (!match(TokenType::Keyword) || previous().value != "for") {
            throw std::runtime_error("Expected 'for' after 'end' for for statement. Got: " + (isAtEnd() ? "EOF" : previous().value));
        }
        return std::make_unique<ForStatement>(varName, std::move(iterable), std::move(body));
    }

};