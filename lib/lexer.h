#pragma once

#include <string>
#include <vector>
#include <cctype>
#include <stdexcept>
#include <iostream>

// типы токенов
enum class TokenType {
    Keyword, Identifier, Number, String, Operator, 
    LParen, RParen, LBracket, RBracket, Comma, Colon,
    EndOfLine, EndOfFile, Semicolon
};

// структура для хранения информации о токене
struct Token {
    TokenType type;
    std::string value;
    size_t line;
    size_t column;
};

// лексический анализатор
class Lexer {
    std::string source;
    size_t pos = 0;
    size_t line = 1;
    size_t column = 1;
    
    // получение текущего символа
    char current() const { return pos < source.size() ? source[pos] : '\0'; }
    
    // получение следующего символа
    char peek() const { return pos + 1 < source.size() ? source[pos + 1] : '\0'; }
    
    // переход к следующему символу с учетом переноса строки
    void advance() { 
        if (current() == '\n') {
            line++;
            column = 1;
        } else {
            column++;
        }
        pos++; 
    }
    
public:
    Lexer(const std::string& src) : source(src) {}
    
    // разбиение исходного кода на токены
    std::vector<Token> tokenize() {
        std::vector<Token> tokens;
        
        while (current() != '\0') {
            if (current() == '\n') {
                tokens.push_back({TokenType::EndOfLine, "\n", line, column});
                advance();
                continue;
            }
            if (isspace(current())) {
                advance();
                continue;
            }
            
            if (current() == '/' && peek() == '/') {
                while (current() != '\n' && current() != '\0') advance();
                continue;
            }
            
            Token token;
            if (isalpha(current())) {
                token = readIdentifierOrKeyword();
            } else if (isdigit(current())) {
                token = readNumber();
            } else if (current() == '"') {
                token = readString();
            } else {
                token = readSymbol();
            }
            tokens.push_back(token);
        }
        
        tokens.push_back({TokenType::EndOfFile, "", line, column});
        return tokens;
    }
    
private:
    // чтение идентификатора или ключевого слова
    Token readIdentifierOrKeyword() {
        size_t start = pos;
        while (isalnum(current()) || current() == '_') advance();
        std::string value = source.substr(start, pos - start);
        
        if (value == "function" || value == "if" || value == "then" || 
            value == "else" || value == "and" || value == "not" || 
            value == "end" || value == "for" || value == "in" || 
            value == "return" || value == "while" || value == "break" || 
            value == "continue" || value == "or" || value == "nil" || 
            value == "true" || value == "false") {
            return {TokenType::Keyword, value, line, column};
        }
        
        return {TokenType::Identifier, value, line, column};
    }
    
    // чтение числового литерала
    Token readNumber() {
        size_t start = pos;
        while (isdigit(current())) advance();
        if (current() == '.') {
            advance();
            while (isdigit(current())) advance();
        }
        if (tolower(current()) == 'e') {
            advance();
            if (current() == '+' || current() == '-') advance();
            while (isdigit(current())) advance();
        }
        return {TokenType::Number, source.substr(start, pos - start), line, column};
    }
    
    // чтение строкового литерала
    Token readString() {
        advance();
        std::string value;
        while (current() != '"' && current() != '\0') {
            if (current() == '\\') {
                advance();
                switch (current()) {
                    case 'n': value += '\n'; break;
                    case 't': value += '\t'; break;
                    case '"': value += '"'; break;
                    case '\\': value += '\\'; break;
                    default: value += current(); break;
                }
            } else {
                value += current();
            }
            advance();
        }
        if (current() == '"') advance();
        return {TokenType::String, value, line, column};
    }
    
    Token readSymbol() {
        char c = current();
        advance();
        switch (c) {
            case '(': return {TokenType::LParen, "(", line, column};
            case ')': return {TokenType::RParen, ")", line, column};
            case '[': return {TokenType::LBracket, "[", line, column};
            case ']': return {TokenType::RBracket, "]", line, column};
            case ',': 
                while (isspace(current()) && current() != '\n') advance();
                return {TokenType::Comma, ",", line, column};
            case ':': return {TokenType::Colon, ":", line, column};
            case '=': 
                if (current() == '=') { 
                    advance(); 
                    return {TokenType::Operator, "==", line, column}; 
                }
                return {TokenType::Operator, "=", line, column};
            case '+': 
                if (current() == '=') {
                    advance();
                    return {TokenType::Operator, "+=", line, column};
                }
                return {TokenType::Operator, "+", line, column};
            case '-': 
                if (current() == '=') {
                    advance();
                    return {TokenType::Operator, "-=", line, column};
                }
                return {TokenType::Operator, "-", line, column};
            case '*': 
                if (current() == '=') {
                    advance();
                    return {TokenType::Operator, "*=", line, column};
                }
                return {TokenType::Operator, "*", line, column};
            case '/': 
                if (current() == '=') {
                    advance();
                    return {TokenType::Operator, "/=", line, column};
                }
                return {TokenType::Operator, "/", line, column};
            case '%': 
                if (current() == '=') {
                    advance();
                    return {TokenType::Operator, "%=", line, column};
                }
                return {TokenType::Operator, "%", line, column};
            case '^': 
                if (current() == '=') {
                    advance();
                    return {TokenType::Operator, "^=", line, column};
                }
                return {TokenType::Operator, "^", line, column};
            case '<':
                if (current() == '=') {
                    advance();
                    return {TokenType::Operator, "<=", line, column};
                }
                return {TokenType::Operator, "<", line, column};
            case '>':
                if (current() == '=') {
                    advance();
                    return {TokenType::Operator, ">=", line, column};
                }
                return {TokenType::Operator, ">", line, column};
            case '!':
                if (current() == '=') {
                    advance();
                    return {TokenType::Operator, "!=", line, column};
                }
                throw std::runtime_error("Expected '=' after '!'");
            case ';': return {TokenType::Semicolon, ";", line, column};
            default: 
                throw std::runtime_error("Unexpected character: " + std::string(1, c));
        }
    }
};