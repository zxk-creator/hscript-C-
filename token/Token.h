#include <string>

#pragma once

enum class TokenType {
    // 单字符：(，)，{，},，.，-，+，;，/，*
    LEFT_PAREN, RIGHT_PAREN, LEFT_BRACE, RIGHT_BRACE,
    COMMA, DOT, MINUS, PLUS, SEMICOLON, SLASH, STAR,

    // 双字符：!，!=，=，==，>，>=，<,<=
    BANG, BANG_EQUAL,
    EQUAL, EQUAL_EQUAL,
    GREATER, GREATER_EQUAL,
    LESS, LESS_EQUAL,

    // 字面量：标识符（变量名，函数名），字符串字面量，数字字面量
    IDENTIFIER, STRING, NUMBER,

    // 关键字：and，class，else，false，fun，for，if，nil，or，print，return，super，this，true，var，while, extends, interface
    AND, CLASS, ELSE, FALSE, FUNCTION, FOR, IF, NIL, OR,
    TRACE, RETURN, SUPER, THIS, TRUE, VAR, WHILE, EXTENDS,
    INTERFACE,

    // 文件结束EOF
    EOFF
};

class Token {
public:
    TokenType type;
    std::string lexeme;
    std::string literal;  // 用 string 存字面量值，数字转字符串
    int line;

    Token(TokenType type, std::string lexeme, std::string literal, int line)
        : type(type), lexeme(lexeme), literal(literal), line(line) {}

    std::string toString() {
        return std::to_string((int)type) + " " + lexeme + " " + literal;
    }
};