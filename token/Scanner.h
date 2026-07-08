
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "Token.h"

// 用于将文本的脚本转化为一种更高级的非文本形式
class Scanner {
private:
    std::string source;
    std::vector<Token> tokens;
    int start = 0;      // 当前词素开始位置
    int current = 0;    // 当前扫描位置
    int line = 1;       // 当前行号

    // 关键字映射表
    std::unordered_map<std::string, TokenType> keywords;
    // 判断是否已扫描到源代码末尾
    bool isAtEnd() { return current >= source.length(); }
    // 消费当前字符并返回，同时向前移动扫描位置
    char advance() { return source[current++]; }
    // 查看当前字符但不消费
    char peek() {
        if (isAtEnd()) return '\0';
        return source[current];
    }
    // 查看下一个字符但不消费
    char peekNext() {
        if (current + 1 >= source.length()) return '\0';
        return source[current + 1];
    }
    // 判断当前字符是否匹配期望字符，匹配则消费并返回 true
    bool match(char expected) {
        if (isAtEnd()) return false;
        if (source[current] != expected) return false;
        current++;
        return true;
    }
    // 添加一个无字面量值的 Token（用于关键字和运算符）
    void addToken(TokenType type) {
        addToken(type, "");
    }
    // 添加一个有字面量值的 Token（用于字符串和数字）
    void addToken(TokenType type, std::string literal) {
        std::string text = source.substr(start, current - start);
        tokens.push_back(Token(type, text, literal, line));
    }
    // 判断字符是否为数字 0-9
    bool isDigit(char c) { return c >= '0' && c <= '9'; }
    // 判断字符是否为字母（a-z, A-Z）或下划线
    bool isAlpha(char c) {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               c == '_';
    }
    // 判断字符是否为字母、数字或下划线
    bool isAlphaNumeric(char c) {
        return isAlpha(c) || isDigit(c);
    }

    // 扫描并处理下一个词法单元
    void scanToken() {
        char c = advance();

        switch (c) {
            // 单字符
            case '(': addToken(TokenType::LEFT_PAREN); break;
            case ')': addToken(TokenType::RIGHT_PAREN); break;
            case '{': addToken(TokenType::LEFT_BRACE); break;
            case '}': addToken(TokenType::RIGHT_BRACE); break;
            case ',': addToken(TokenType::COMMA); break;
            case '.': addToken(TokenType::DOT); break;
            case '-': addToken(TokenType::MINUS); break;
            case '+': addToken(TokenType::PLUS); break;
            case ';': addToken(TokenType::SEMICOLON); break;
            case '*': addToken(TokenType::STAR); break;

            // 双字符运算符
            case '!':
                addToken(match('=') ? TokenType::BANG_EQUAL : TokenType::BANG);
                break;
            case '=':
                addToken(match('=') ? TokenType::EQUAL_EQUAL : TokenType::EQUAL);
                break;
            case '<':
                addToken(match('=') ? TokenType::LESS_EQUAL : TokenType::LESS);
                break;
            case '>':
                addToken(match('=') ? TokenType::GREATER_EQUAL : TokenType::GREATER);
                break;

            case '/':
                if (match('/')) {
                    // 注释，读到行尾
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    addToken(TokenType::SLASH);
                }
                break;

            // 空格和换行
            case ' ':
            case '\r':
            case '\t':
                break;  // 忽略空白
            case '\n':
                line++;
                break;

            case '"':
                string();
                break;

            default:
                if (isDigit(c)) {
                    number();
                } else if (isAlpha(c)) {
                    identifier();
                } else {
                    // 报错
                    std::cerr << "[行 " << line << "] 错误: 未预期的Token\n";
                }
                break;
        }
    }

    // 处理字符串
    void string() {
        while (peek() != '"' && !isAtEnd()) {
            if (peek() == '\n') line++;
            advance();
        }

        if (isAtEnd()) {
            std::cerr << "[行 " << line << "] 错误: 无法中断的字符串.\n";
            return;
        }

        // 消费关闭的 "
        advance();

        // 去掉引号
        std::string value = source.substr(start + 1, current - start - 2);
        addToken(TokenType::STRING, value);
    }

    // 处理数字
    void number() {
        while (isDigit(peek())) advance();

        // 小数部分
        if (peek() == '.' && isDigit(peekNext())) {
            advance();  // 消费 .
            while (isDigit(peek())) advance();
        }

        std::string value = source.substr(start, current - start);
        addToken(TokenType::NUMBER, value);
    }

    // 处理标识符和关键字
    void identifier() {
        while (isAlphaNumeric(peek())) advance();

        std::string text = source.substr(start, current - start);

        auto it = keywords.find(text);
        TokenType type = (it != keywords.end()) ? it->second : TokenType::IDENTIFIER;
        addToken(type);
    }

public:
    Scanner(std::string source) : source(source) {
        keywords["&&"]       = TokenType::AND;
        keywords["和"]       = TokenType::AND;

        keywords["class"]    = TokenType::CLASS;
        keywords["类"]       = TokenType::CLASS;

        keywords["else"]     = TokenType::ELSE;
        keywords["否则"]     = TokenType::ELSE;

        keywords["false"]    = TokenType::FALSE;
        keywords["假"]       = TokenType::FALSE;

        keywords["for"]      = TokenType::FOR;
        keywords["循环"]     = TokenType::FOR;

        keywords["function"] = TokenType::FUNCTION;
        keywords["函数"]     = TokenType::FUNCTION;

        keywords["if"]       = TokenType::IF;
        keywords["如果"]     = TokenType::IF;

        keywords["null"]     = TokenType::NIL;
        keywords["空"]       = TokenType::NIL;

        keywords["||"]       = TokenType::OR;
        keywords["或"]       = TokenType::OR;

        keywords["打印"]     = TokenType::TRACE;

        keywords["return"]   = TokenType::RETURN;
        keywords["返回"]     = TokenType::RETURN;

        keywords["super"]    = TokenType::SUPER;
        keywords["父类"]     = TokenType::SUPER;

        keywords["this"]     = TokenType::THIS;
        keywords["自己"]     = TokenType::THIS;

        keywords["true"]     = TokenType::TRUE;
        keywords["真"]       = TokenType::TRUE;

        keywords["var"]      = TokenType::VAR;
        keywords["变量"]     = TokenType::VAR;

        keywords["while"]    = TokenType::WHILE;
        keywords["当"]       = TokenType::WHILE;
    }

    std::vector<Token> scanTokens() {
        while (!isAtEnd()) {
            start = current;
            scanToken();
        }

        tokens.push_back(Token(TokenType::EOFF, "", "", line));
        return tokens;
    }
};