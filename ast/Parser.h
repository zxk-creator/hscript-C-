
#pragma once
#include <vector>
#include <memory>
#include <stdexcept>
#include <initializer_list>
#include "Expr.h"

// 用于把Token列表转换成AST
class Parser {
    std::vector<Token> tokens;
    int current = 0;

    bool isAtEnd() const {
        return peek().type == TokenType::EOFF;
    }

    Token peek() const {
        return tokens[current];
    }

    Token previous() const {
        return tokens[current - 1];
    }

    Token advance() {
        if (!isAtEnd()) current++;
        return previous();
    }

    bool check(TokenType type) const {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    bool match(std::initializer_list<TokenType> types) {
        for (TokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    Token consume(TokenType type, const std::string& message) {
        if (check(type)) return advance();
        throw std::runtime_error(message);
    }

    // expression → assignment
    std::unique_ptr<Expr> expression() {
        return assignment();
    }

    // assignment → IDENTIFIER "=" assignment | or
    std::unique_ptr<Expr> assignment() {
        auto expr = or_();

        if (match({TokenType::EQUAL})) {
            Token equals = previous();
            auto value = assignment();

            // 检查左侧是否是变量
            if (auto* varExpr = dynamic_cast<Variable*>(expr.get())) {
                Token name = varExpr->name;
                return std::make_unique<Assign>(name, std::move(value));
            }

            throw std::runtime_error("无效的赋值目标。");
        }

        return expr;
    }

    // or → and ( "or" and )*
    std::unique_ptr<Expr> or_() {
        auto expr = and_();

        while (match({TokenType::OR})) {
            Token op = previous();
            auto right = and_();
            expr = std::make_unique<Logical>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // and → equality ( "and" equality )*
    std::unique_ptr<Expr> and_() {
        auto expr = equality();

        while (match({TokenType::AND})) {
            Token op = previous();
            auto right = equality();
            expr = std::make_unique<Logical>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // equality → comparison (("!=" | "==") comparison)*
    std::unique_ptr<Expr> equality() {
        std::unique_ptr<Expr> expr = comparison();

        while (match({TokenType::BANG_EQUAL, TokenType::EQUAL_EQUAL})) {
            Token op = previous();
            std::unique_ptr<Expr> right = comparison();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // comparison → term ((">" | ">=" | "<" | "<=") term)*
    std::unique_ptr<Expr> comparison() {
        std::unique_ptr<Expr> expr = term();

        while (match({TokenType::GREATER, TokenType::GREATER_EQUAL,
                      TokenType::LESS, TokenType::LESS_EQUAL})) {
            Token op = previous();
            std::unique_ptr<Expr> right = term();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // term → factor (("-" | "+") factor)*
    std::unique_ptr<Expr> term() {
        std::unique_ptr<Expr> expr = factor();

        while (match({TokenType::MINUS, TokenType::PLUS})) {
            Token op = previous();
            std::unique_ptr<Expr> right = factor();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // factor → unary (("/" | "*") unary)*
    std::unique_ptr<Expr> factor() {
        std::unique_ptr<Expr> expr = unary();

        while (match({TokenType::SLASH, TokenType::STAR})) {
            Token op = previous();
            std::unique_ptr<Expr> right = unary();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // unary → ("!" | "-") unary | primary
    std::unique_ptr<Expr> unary() {
        if (match({TokenType::BANG, TokenType::MINUS})) {
            Token op = previous();
            std::unique_ptr<Expr> right = unary();
            return std::make_unique<Unary>(op, std::move(right));
        }

        return primary();
    }

    // primary → NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" | IDENTIFIER
    std::unique_ptr<Expr> primary() {
        if (match({TokenType::FALSE})) {
            return std::make_unique<Literal>(Dynamic(false));
        }
        if (match({TokenType::TRUE})) {
            return std::make_unique<Literal>(Dynamic(true));
        }
        if (match({TokenType::NIL})) {
            return std::make_unique<Literal>(Dynamic());
        }

        // 数字或字符串
        if (match({TokenType::NUMBER, TokenType::STRING})) {
            Token token = previous();
            if (token.type == TokenType::NUMBER) {
                double value = std::stod(token.lexeme);
                return std::make_unique<Literal>(Dynamic(value));
            } else {
                return std::make_unique<Literal>(Dynamic(token.literal));
            }
        }

        // 变量
        if (match({TokenType::IDENTIFIER})) {
            return std::make_unique<Variable>(previous());
        }

        // 括号分组
        if (match({TokenType::LEFT_PAREN})) {
            std::unique_ptr<Expr> expr = expression();
            consume(TokenType::RIGHT_PAREN, "表达式后期望 ')'。");
            return std::make_unique<Grouping>(std::move(expr));
        }

        throw std::runtime_error("期望一个表达式。");
    }

    // declaration → varDecl | statement
    std::unique_ptr<Stmt> declaration() {
        try {
            if (match({TokenType::VAR})) {
                return varDeclaration();
            }
            return statement();
        } catch (const std::runtime_error& e) {
            synchronize();
            return nullptr;
        }
    }

    // statement → exprStmt | ifStmt | whileStmt | forStmt | block
    std::unique_ptr<Stmt> statement() {
        if (match({TokenType::FOR})) {
            return forStatement();
        }
        if (match({TokenType::IF})) {
            return ifStatement();
        }
        if (match({TokenType::WHILE})) {
            return whileStatement();
        }
        if (match({TokenType::LEFT_BRACE})) {
            return std::make_unique<BlockStmt>(block());
        }
        return expressionStatement();
    }

    // varDecl → "var" IDENTIFIER ("=" expression)? ";"
    std::unique_ptr<Stmt> varDeclaration() {
        Token name = consume(TokenType::IDENTIFIER, "期望变量名。");

        std::unique_ptr<Expr> initializer = nullptr;
        if (match({TokenType::EQUAL})) {
            initializer = expression();
        }

        consume(TokenType::SEMICOLON, "变量声明后期望 ';'。");
        return std::make_unique<VarStmt>(name, std::move(initializer));
    }

    // expressionStmt → expression ";"
    std::unique_ptr<Stmt> expressionStatement() {
        auto expr = expression();
        consume(TokenType::SEMICOLON, "表达式后期望 ';'。");
        return std::make_unique<ExpressionStmt>(std::move(expr));
    }

    // ifStmt → "if" "(" expression ")" statement ("else" statement)?
    std::unique_ptr<Stmt> ifStatement() {
        consume(TokenType::LEFT_PAREN, "if 后期望 '('。");
        auto condition = expression();
        consume(TokenType::RIGHT_PAREN, "if 条件后期望 ')'。");

        auto thenBranch = statement();

        std::unique_ptr<Stmt> elseBranch = nullptr;
        if (match({TokenType::ELSE})) {
            elseBranch = statement();
        }

        return std::make_unique<IfStmt>(
            std::move(condition),
            std::move(thenBranch),
            std::move(elseBranch)
        );
    }

    // whileStmt → "while" "(" expression ")" statement
    std::unique_ptr<Stmt> whileStatement() {
        consume(TokenType::LEFT_PAREN, "while后期望 '('。");
        auto condition = expression();
        consume(TokenType::RIGHT_PAREN, "while条件后期望 ')'。");

        auto body = statement();

        return std::make_unique<WhileStmt>(
            std::move(condition),
            std::move(body)
        );
    }

    // forStmt → "for" "(" (varDecl | exprStmt | ";") expression? ";" expression? ")" statement
    // 脱糖为while循环
    std::unique_ptr<Stmt> forStatement() {
        consume(TokenType::LEFT_PAREN, "for 后期望 '('。");

        // 初始化式
        std::unique_ptr<Stmt> initializer = nullptr;
        if (match({TokenType::SEMICOLON})) {
            // 空初始化式
            initializer = nullptr;
        } else if (match({TokenType::VAR})) {
            initializer = varDeclaration();
        } else {
            initializer = expressionStatement();
        }

        // 条件式
        std::unique_ptr<Expr> condition = nullptr;
        if (!check(TokenType::SEMICOLON)) {
            condition = expression();
        }
        consume(TokenType::SEMICOLON, "循环条件后期望 ';'。");

        // 增量式
        std::unique_ptr<Expr> increment = nullptr;
        if (!check(TokenType::RIGHT_PAREN)) {
            increment = expression();
        }
        consume(TokenType::RIGHT_PAREN, "for 子句后期望 ')'。");

        // 循环体
        auto body = statement();

        // 如果有增量式，追加到循环体末尾
        if (increment) {
            std::vector<std::unique_ptr<Stmt>> stmts;
            stmts.push_back(std::move(body));
            stmts.push_back(std::make_unique<ExpressionStmt>(std::move(increment)));
            body = std::make_unique<BlockStmt>(std::move(stmts));
        }

        // 如果条件为空，用 true 代替（无限循环）
        if (!condition) {
            condition = std::make_unique<Literal>(Dynamic(true));
        }

        // 构建while循环
        body = std::make_unique<WhileStmt>(
            std::move(condition),
            std::move(body)
        );

        // 如果有初始化式，放在循环前面
        if (initializer) {
            std::vector<std::unique_ptr<Stmt>> stmts;
            stmts.push_back(std::move(initializer));
            stmts.push_back(std::move(body));
            body = std::make_unique<BlockStmt>(std::move(stmts));
        }

        return body;
    }

    // block → "{" declaration* "}"
    std::vector<std::unique_ptr<Stmt>> block() {
        std::vector<std::unique_ptr<Stmt>> statements;

        while (!check(TokenType::RIGHT_BRACE) && !isAtEnd()) {
            auto stmt = declaration();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        }

        consume(TokenType::RIGHT_BRACE, "代码块后期望 '}'。");
        return statements;
    }

    // 同步
    void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous().type == TokenType::SEMICOLON) return;

            switch (peek().type) {
                case TokenType::CLASS:
                case TokenType::FUNCTION:
                case TokenType::VAR:
                case TokenType::FOR:
                case TokenType::IF:
                case TokenType::WHILE:
                case TokenType::RETURN:
                    return;
                default:
                    break;
            }

            advance();
        }
    }

public:
    Parser(const std::vector<Token>& tokens) : tokens(tokens) {}

    // 程序主入口！解析整个程序，返回语句列表
    std::vector<std::unique_ptr<Stmt>> parse() {
        std::vector<std::unique_ptr<Stmt>> statements;
        while (!isAtEnd()) {
            auto stmt = declaration();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        }
        return statements;
    }
};