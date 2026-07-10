
#pragma once
#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include "Token.h"

// 用于将文本的脚本转化为一种更高级的非纯文本形式
class Scanner {
    std::string source;
    std::vector<Token> tokens;
    int start = 0;      // 当前词素开始位置
    int current = 0;    // 当前扫描位置
    int line = 1;       // 当前行号
    // 注：start和current的关系是，标注一个完整的词素，比如数字、字符串、标识符，以及运算符和括号。只需要current - start就能得到词素长度

    // 关键字映射表
    std::unordered_map<std::string, ETokenType> keywords;
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
    // 判断当前字符是否匹配期望字符，匹配则消费并返回true
    bool match(char expected) {
        if (isAtEnd()) return false;
        if (source[current] != expected) return false;
        current++;
        return true;
    }
    // 添加一个无字面量值的Token
    void addToken(ETokenType type) {
        addToken(type, "");
    }
    // 添加一个有字面量值的Token用于字符串和数字
    void addToken(ETokenType type, std::string literal) {
        std::string text = source.substr(start, current - start);
        tokens.push_back(Token(type, text, literal, line));
    }
    // 判断字符是否为数字 0-9
    bool isDigit(char c) { return c >= '0' && c <= '9'; }
    // 判断字符是否为字母a-z,A-Z或下划线
    bool isAlpha(char c) {
        return (c >= 'a' && c <= 'z') ||
               (c >= 'A' && c <= 'Z') ||
               c == '_' ||
               (c & 0x80);  // UTF-8 多字节字符（中文等）
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
            case '(': addToken(ETokenType::LEFT_PAREN); break;
            case ')': addToken(ETokenType::RIGHT_PAREN); break;
            case '{': addToken(ETokenType::LEFT_BRACE); break;
            case '}': addToken(ETokenType::RIGHT_BRACE); break;
            case ',': addToken(ETokenType::COMMA); break;
            case '.': addToken(ETokenType::DOT); break;
            case '-': addToken(ETokenType::MINUS); break;
            case '+': addToken(ETokenType::PLUS); break;
            case ';': addToken(ETokenType::SEMICOLON); break;
            case '*': addToken(ETokenType::STAR); break;
            case ':': addToken(ETokenType::COLON); break;

            // 双字符运算符（为什么用match，因为switch所用的c已经被消费了）
            case '!':
                addToken(match('=') ? ETokenType::BANG_EQUAL : ETokenType::BANG);
                break;
            case '=':
                addToken(match('=') ? ETokenType::EQUAL_EQUAL : ETokenType::EQUAL);
                break;
            case '<':
                addToken(match('=') ? ETokenType::LESS_EQUAL : ETokenType::LESS);
                break;
            case '>':
                addToken(match('=') ? ETokenType::GREATER_EQUAL : ETokenType::GREATER);
                break;

            case '/':
                if (match('/')) {
                    // 注释，读到行尾
                    while (peek() != '\n' && !isAtEnd()) advance();
                } else {
                    addToken(ETokenType::SLASH);
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

            case '@':
                addToken(ETokenType::AT);
                break;

            default:
                if (isDigit(c)) {
                    number();
                } else if (isAlpha(c)) {
                    identifier();
                } else {
                    std::cerr << "[行 " << line << "] 错误: 非法的字符" << c << std::endl;
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
            std::cerr << "[行 " << line << "] 错误: 无法中断的字符串。你是否忘记了另一个\"？\n";
            return;
        }

        // 消费另一头关闭的 "
        advance();

        // 去掉引号
        std::string value = source.substr(start + 1, current - start - 2);
        addToken(ETokenType::STRING, value);
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
        addToken(ETokenType::NUMBER, value);
    }

    // 处理标识符和关键字
    void identifier() {
        while (isAlphaNumeric(peek())) advance();

        std::string text = source.substr(start, current - start);

        auto it = keywords.find(text);
        ETokenType type = (it != keywords.end()) ? it->second : ETokenType::IDENTIFIER;
        addToken(type);
    }

public:
    Scanner(std::string source) : source(source) {
        keywords["&&"]        = ETokenType::AND;
        keywords["和"]        = ETokenType::AND;

        keywords["class"]     = ETokenType::CLASS;
        keywords["类"]        = ETokenType::CLASS;

        keywords["else"]      = ETokenType::ELSE;
        keywords["否则"]      = ETokenType::ELSE;

        keywords["false"]     = ETokenType::FALSE;
        keywords["假"]        = ETokenType::FALSE;

        keywords["for"]       = ETokenType::FOR;
        keywords["循环"]      = ETokenType::FOR;

        keywords["function"]  = ETokenType::FUNCTION;
        keywords["函数"]      = ETokenType::FUNCTION;

        keywords["if"]        = ETokenType::IF;
        keywords["如果"]      = ETokenType::IF;

        keywords["null"]      = ETokenType::NIL;
        keywords["空"]        = ETokenType::NIL;

        keywords["||"]        = ETokenType::OR;
        keywords["或"]        = ETokenType::OR;

        keywords["return"]    = ETokenType::RETURN;
        keywords["返回"]      = ETokenType::RETURN;

        keywords["super"]     = ETokenType::SUPER;
        keywords["父类"]      = ETokenType::SUPER;

        keywords["this"]      = ETokenType::THIS;
        keywords["自己"]      = ETokenType::THIS;

        keywords["true"]      = ETokenType::TRUE;
        keywords["真"]        = ETokenType::TRUE;

        keywords["var"]       = ETokenType::VAR;
        keywords["变量"]      = ETokenType::VAR;

        keywords["while"]     = ETokenType::WHILE;
        keywords["当"]        = ETokenType::WHILE;

        keywords["extends"]   = ETokenType::EXTENDS;
        keywords["继承"]      = ETokenType::EXTENDS;

        keywords["new"]       = ETokenType::NEW;
        keywords["新的"]      = ETokenType::NEW;

        keywords["public"]    = ETokenType::PUBLIC;
        keywords["公开"]      = ETokenType::PUBLIC;

        keywords["private"]   = ETokenType::PRIVATE;
        keywords["私有"]      = ETokenType::PRIVATE;

        keywords["protected"] = ETokenType::PROTECTED;
        keywords["受保护"]    = ETokenType::PROTECTED;

        keywords["override"]  = ETokenType::OVERRIDE;
        keywords["重写"]      = ETokenType::OVERRIDE;

        keywords["final"]     = ETokenType::FINAL;
        keywords["最终"]      = ETokenType::FINAL;

        keywords["inline"]    = ETokenType::INLINE;
        keywords["内联"]      = ETokenType::INLINE;

        keywords["macro"]     = ETokenType::MACRO;
        keywords["宏"]        = ETokenType::MACRO;

        keywords["static"]    = ETokenType::STATIC;
        keywords["静态"]      = ETokenType::STATIC;
    }

    std::vector<Token> scanTokens() {
        while (!isAtEnd()) {
            start = current;
            scanToken();
        }

        // 结束了，加个文件结束标识，多个F是为了防止与C++关键字冲突
        tokens.push_back(Token(ETokenType::EOFF, "", "", line));
        // 返回扫描好的token列表
        return tokens;
    }
};