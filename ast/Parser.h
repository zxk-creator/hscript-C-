
#pragma once
#include <vector>
#include <memory>
#include <stdexcept>
#include <initializer_list>
#include "Expr.h"

// 用于把Token列表转换并构建AST
class Parser {
    std::vector<Token> tokens;
    int current = 0;

    bool isAtEnd() const {
        return peek().type == ETokenType::EOFF;
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

    bool check(ETokenType type) const {
        if (isAtEnd()) return false;
        return peek().type == type;
    }

    bool match(std::initializer_list<ETokenType> types) {
        for (ETokenType type : types) {
            if (check(type)) {
                advance();
                return true;
            }
        }
        return false;
    }

    Token consume(ETokenType type, const std::string& message) {
        if (check(type)) return advance();
        throw std::runtime_error(message);
    }

    // expression → assignment
    std::unique_ptr<Expr> expression() {
        return assignment();
    }

    // assignment → (call ".")? IDENTIFIER "=" assignment | or
    std::unique_ptr<Expr> assignment() {
        auto expr = or_();

        if (match({ETokenType::EQUAL})) {
            Token equals = previous();
            auto value = assignment();

            // 变量赋值: a = value
            if (auto* varExpr = dynamic_cast<Variable*>(expr.get())) {
                Token name = varExpr->name;
                return std::make_unique<Assign>(name, std::move(value));
            }

            // 属性赋值: obj.field = value
            if (auto* getExpr = dynamic_cast<Get*>(expr.get())) {
                Token name = getExpr->name;
                auto object = std::move(getExpr->object);
                return std::make_unique<Set>(std::move(object), name, std::move(value));
            }

            throw std::runtime_error("无效的赋值目标。");
        }

        return expr;
    }

    // or → and ( "or" and )*
    std::unique_ptr<Expr> or_() {
        auto expr = and_();

        while (match({ETokenType::OR})) {
            Token op = previous();
            auto right = and_();
            expr = std::make_unique<Logical>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // and → equality ( "and" equality )*
    std::unique_ptr<Expr> and_() {
        auto expr = equality();

        while (match({ETokenType::AND})) {
            Token op = previous();
            auto right = equality();
            expr = std::make_unique<Logical>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // equality → comparison (("!=" | "==") comparison)*
    std::unique_ptr<Expr> equality() {
        std::unique_ptr<Expr> expr = comparison();

        while (match({ETokenType::BANG_EQUAL, ETokenType::EQUAL_EQUAL})) {
            Token op = previous();
            std::unique_ptr<Expr> right = comparison();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // comparison → term ((">" | ">=" | "<" | "<=") term)*
    std::unique_ptr<Expr> comparison() {
        std::unique_ptr<Expr> expr = term();

        while (match({ETokenType::GREATER, ETokenType::GREATER_EQUAL,
                      ETokenType::LESS, ETokenType::LESS_EQUAL})) {
            Token op = previous();
            std::unique_ptr<Expr> right = term();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // term → factor (("-" | "+") factor)*
    std::unique_ptr<Expr> term() {
        std::unique_ptr<Expr> expr = factor();

        while (match({ETokenType::MINUS, ETokenType::PLUS})) {
            Token op = previous();
            std::unique_ptr<Expr> right = factor();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // factor → unary (("/" | "*") unary)*
    std::unique_ptr<Expr> factor() {
        std::unique_ptr<Expr> expr = unary();

        while (match({ETokenType::SLASH, ETokenType::STAR})) {
            Token op = previous();
            std::unique_ptr<Expr> right = unary();
            expr = std::make_unique<Binary>(std::move(expr), op, std::move(right));
        }

        return expr;
    }

    // unary → ("!" | "-") unary | call
    std::unique_ptr<Expr> unary() {
        if (match({ETokenType::BANG, ETokenType::MINUS})) {
            Token op = previous();
            std::unique_ptr<Expr> right = unary();
            return std::make_unique<Unary>(op, std::move(right));
        }

        return call();
    }

    // call → primary ( "(" arguments? ")" | "." IDENTIFIER )*
    std::unique_ptr<Expr> call() {
        auto expr = primary();

        while (true) {
            if (match({ETokenType::LEFT_PAREN})) {
                expr = finishCall(std::move(expr));
            } else if (match({ETokenType::DOT})) {
                Token name = consume(ETokenType::IDENTIFIER, "属性名期望。");
                expr = std::make_unique<Get>(std::move(expr), name);
            } else {
                break;
            }
        }

        return expr;
    }

    // 完成函数调用解析: ( arguments? )
    std::unique_ptr<Expr> finishCall(std::unique_ptr<Expr> callee) {
        std::vector<std::unique_ptr<Expr>> arguments;
        if (!check(ETokenType::RIGHT_PAREN)) {
            do {
                if (arguments.size() >= 255) {
                    throw std::runtime_error("函数参数不能超过255个");
                }
                arguments.push_back(expression());
            } while (match({ETokenType::COMMA}));
        }

        Token paren = consume(ETokenType::RIGHT_PAREN, "参数列表后期望 ')'");

        return std::make_unique<Call>(std::move(callee), paren, std::move(arguments));
    }

    // primary → NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")" | IDENTIFIER | "this" | "super" | "new" IDENTIFIER "(" arguments? ")"
    std::unique_ptr<Expr> primary() {
        if (match({ETokenType::FALSE})) {
            return std::make_unique<Literal>(Dynamic(false));
        }
        if (match({ETokenType::TRUE})) {
            return std::make_unique<Literal>(Dynamic(true));
        }
        if (match({ETokenType::NIL})) {
            return std::make_unique<Literal>(Dynamic());
        }

        // 数字或字符串
        if (match({ETokenType::NUMBER, ETokenType::STRING})) {
            Token token = previous();
            if (token.type == ETokenType::NUMBER) {
                double value = std::stod(token.lexeme);
                return std::make_unique<Literal>(Dynamic(value));
            } else {
                return std::make_unique<Literal>(Dynamic(token.literal));
            }
        }

        // 变量
        if (match({ETokenType::IDENTIFIER})) {
            return std::make_unique<Variable>(previous());
        }

        // 括号分组
        if (match({ETokenType::LEFT_PAREN})) {
            std::unique_ptr<Expr> expr = expression();
            consume(ETokenType::RIGHT_PAREN, "表达式后期望 ')'。");
            return std::make_unique<Grouping>(std::move(expr));
        }

        // this 关键字
        if (match({ETokenType::THIS})) {
            return std::make_unique<This>(previous());
        }

        // super 关键字
        if (match({ETokenType::SUPER})) {
            return std::make_unique<Super>(previous());
        }

        // new 表达式: new ClassName(args)
        if (match({ETokenType::NEW})) {
            Token className = consume(ETokenType::IDENTIFIER, "new后期望类名。");
            consume(ETokenType::LEFT_PAREN, "类名后期望 '('。");
            std::vector<std::unique_ptr<Expr>> arguments;
            if (!check(ETokenType::RIGHT_PAREN)) {
                do {
                    if (arguments.size() >= 255) {
                        throw std::runtime_error("构造参数不能超过255个");
                    }
                    arguments.push_back(expression());
                } while (match({ETokenType::COMMA}));
            }
            consume(ETokenType::RIGHT_PAREN, "参数列表后期望 ')'。");
            return std::make_unique<NewExpr>(className, std::move(arguments));
        }

        throw std::runtime_error("期望一个表达式。");
    }

    // declaration → annotations? modifiers? classDecl | funDecl | varDecl | statement
    std::unique_ptr<Stmt> declaration() {
        try {
            // 消费注解 (被忽略)
            consumeAnnotations();

            // 消费访问修饰符 (被忽略)
            consumeClassModifiers();

            if (match({ETokenType::CLASS})) {
                return classDeclaration();
            }
            if (match({ETokenType::FUNCTION})) {
                return functionDeclaration();
            }
            if (match({ETokenType::VAR})) {
                return varDeclaration();
            }
            return statement();
        } catch (const std::runtime_error& e) {
            synchronize();
            return nullptr;
        }
    }

    // 类声明: "class" IDENTIFIER ("extends" IDENTIFIER)? "{" method* "}"
    std::unique_ptr<Stmt> classDeclaration() {
        Token name = consume(ETokenType::IDENTIFIER, "类名期望。");

        // 父类
        std::unique_ptr<Expr> superclass = nullptr;
        if (match({ETokenType::EXTENDS})) {
            Token superName = consume(ETokenType::IDENTIFIER, "父类名期望。");
            superclass = std::make_unique<Variable>(superName);
        }

        consume(ETokenType::LEFT_BRACE, "类体前期望 '{'。");

        // 解析方法
        std::vector<std::unique_ptr<FunctionStmt>> methods;
        while (!check(ETokenType::RIGHT_BRACE) && !isAtEnd()) {
            // 注解和修饰符
            consumeAnnotations();
            consumeMethodModifiers();

            if (match({ETokenType::FUNCTION})) {
                methods.push_back(std::unique_ptr<FunctionStmt>(
                    static_cast<FunctionStmt*>(functionDeclaration().release())));
            } else {
                throw std::runtime_error("类体内只允许方法定义。");
            }
        }

        consume(ETokenType::RIGHT_BRACE, "类体后期望 '}'。");

        return std::make_unique<ClassStmt>(name, std::move(superclass), std::move(methods));
    }

    // 消费注解: @xxx 或 @xxx(...) （被忽略）
    void consumeAnnotations() {
        while (match({ETokenType::AT})) {
            consume(ETokenType::IDENTIFIER, "注解后期望标识符。");
            if (match({ETokenType::LEFT_PAREN})) {
                int depth = 1;
                while (depth > 0 && !isAtEnd()) {
                    if (peek().type == ETokenType::LEFT_PAREN) depth++;
                    else if (peek().type == ETokenType::RIGHT_PAREN) depth--;
                    else if (peek().type == ETokenType::EOFF) break;
                    advance();
                }
            }
        }
    }

    // 消费类修饰符: public, private, protected, final （被忽略）
    void consumeClassModifiers() {
        while (match({ETokenType::PUBLIC, ETokenType::PRIVATE, ETokenType::PROTECTED,
                      ETokenType::FINAL, ETokenType::INLINE, ETokenType::MACRO})) {
            // 直接忽略
        }
    }

    // 消费方法修饰符: public, private, protected, override, final, inline, macro, static （被忽略）
    void consumeMethodModifiers() {
        while (match({ETokenType::PUBLIC, ETokenType::PRIVATE, ETokenType::PROTECTED,
                      ETokenType::OVERRIDE, ETokenType::FINAL, ETokenType::INLINE,
                      ETokenType::MACRO, ETokenType::STATIC})) {
            // 直接忽略
        }
    }

    // 消费函数名（可以是标识符或 "new" 关键字）
    Token consumeFunctionName() {
        if (check(ETokenType::IDENTIFIER) || check(ETokenType::NEW)) {
            return advance();
        }
        throw std::runtime_error("函数名期望。");
    }

    // functionDeclaration → "function" IDENTIFIER "(" parameters? ")" typeAnnotation? block
    // 也支持构造函数名为 "new"（虽然是关键字）
    std::unique_ptr<Stmt> functionDeclaration() {
        Token name = consumeFunctionName();

        consume(ETokenType::LEFT_PAREN, "函数名后期望 '('。");
        std::vector<Token> params;
        if (!check(ETokenType::RIGHT_PAREN)) {
            do {
                if (params.size() >= 255) {
                    throw std::runtime_error("函数参数不能超过255个");
                }
                Token paramName = consume(ETokenType::IDENTIFIER, "参数名期望。");
                consumeTypeAnnotation();  // 参数类型注解（忽略）
                params.push_back(paramName);
            } while (match({ETokenType::COMMA}));
        }
        consume(ETokenType::RIGHT_PAREN, "参数列表后期望 ')'。");

        consumeTypeAnnotation();  // 返回值类型注解（忽略）

        consume(ETokenType::LEFT_BRACE, "函数体前期望 '{'。");
        auto body = block();

        return std::make_unique<FunctionStmt>(name, std::move(params), std::move(body));
    }

    // 类型注解: ":" IDENTIFIER ("<" ... ">")?
    // 纯语法支持，不做类型检查，直接忽略
    void consumeTypeAnnotation() {
        if (match({ETokenType::COLON})) {
            consume(ETokenType::IDENTIFIER, "类型注解后期望类型名。");
            // 支持泛型: Array<Int>, Map<String, Array<Int>>
            if (match({ETokenType::LESS})) {
                int depth = 1;
                while (depth > 0 && !isAtEnd()) {
                    if (peek().type == ETokenType::LESS) depth++;
                    else if (peek().type == ETokenType::GREATER) depth--;
                    else if (peek().type == ETokenType::EOFF) break;
                    advance();
                }
            }
        }
    }

    // statement → exprStmt | ifStmt | whileStmt | forStmt | returnStmt | block
    std::unique_ptr<Stmt> statement() {
        if (match({ETokenType::FOR})) {
            return forStatement();
        }
        if (match({ETokenType::IF})) {
            return ifStatement();
        }
        if (match({ETokenType::WHILE})) {
            return whileStatement();
        }
        if (match({ETokenType::RETURN})) {
            return returnStatement();
        }
        if (match({ETokenType::LEFT_BRACE})) {
            return std::make_unique<BlockStmt>(block());
        }
        return expressionStatement();
    }

    // returnStatement → "return" expression? ";"
    std::unique_ptr<Stmt> returnStatement() {
        Token keyword = previous();
        std::unique_ptr<Expr> value = nullptr;
        if (!check(ETokenType::SEMICOLON)) {
            value = expression();
        }
        consume(ETokenType::SEMICOLON, "return后期望';'。");
        return std::make_unique<ReturnStmt>(keyword, std::move(value));
    }

    // varDecl → "var" IDENTIFIER ("=" expression)? ";"
    std::unique_ptr<Stmt> varDeclaration() {
        Token name = consume(ETokenType::IDENTIFIER, "期望变量名。");

        consumeTypeAnnotation();  // 变量类型注解（忽略）

        std::unique_ptr<Expr> initializer = nullptr;
        if (match({ETokenType::EQUAL})) {
            initializer = expression();
        }

        consume(ETokenType::SEMICOLON, "变量声明后期望 ';'。");
        return std::make_unique<VarStmt>(name, std::move(initializer));
    }

    // expressionStmt → expression ";"
    std::unique_ptr<Stmt> expressionStatement() {
        auto expr = expression();
        consume(ETokenType::SEMICOLON, "表达式后期望 ';'。");
        return std::make_unique<ExpressionStmt>(std::move(expr));
    }

    // ifStmt → "if" "(" expression ")" statement ("else" statement)?
    std::unique_ptr<Stmt> ifStatement() {
        consume(ETokenType::LEFT_PAREN, "if 后期望 '('。");
        auto condition = expression();
        consume(ETokenType::RIGHT_PAREN, "if 条件后期望 ')'。");

        auto thenBranch = statement();

        std::unique_ptr<Stmt> elseBranch = nullptr;
        if (match({ETokenType::ELSE})) {
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
        consume(ETokenType::LEFT_PAREN, "while后期望 '('。");
        auto condition = expression();
        consume(ETokenType::RIGHT_PAREN, "while条件后期望 ')'。");

        auto body = statement();

        return std::make_unique<WhileStmt>(
            std::move(condition),
            std::move(body)
        );
    }

    // forStmt → "for" "(" (varDecl | exprStmt | ";") expression? ";" expression? ")" statement
    // 脱糖为while循环
    std::unique_ptr<Stmt> forStatement() {
        consume(ETokenType::LEFT_PAREN, "for 后期望 '('。");

        // 初始化式
        std::unique_ptr<Stmt> initializer = nullptr;
        if (match({ETokenType::SEMICOLON})) {
            // 空初始化式
            initializer = nullptr;
        } else if (match({ETokenType::VAR})) {
            initializer = varDeclaration();
        } else {
            initializer = expressionStatement();
        }

        // 条件式
        std::unique_ptr<Expr> condition = nullptr;
        if (!check(ETokenType::SEMICOLON)) {
            condition = expression();
        }
        consume(ETokenType::SEMICOLON, "循环条件后期望 ';'。");

        // 增量式
        std::unique_ptr<Expr> increment = nullptr;
        if (!check(ETokenType::RIGHT_PAREN)) {
            increment = expression();
        }
        consume(ETokenType::RIGHT_PAREN, "for 子句后期望 ')'。");

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

        while (!check(ETokenType::RIGHT_BRACE) && !isAtEnd()) {
            auto stmt = declaration();
            if (stmt) {
                statements.push_back(std::move(stmt));
            }
        }

        consume(ETokenType::RIGHT_BRACE, "代码块后期望 '}'。");
        return statements;
    }

    // 同步
    void synchronize() {
        advance();

        while (!isAtEnd()) {
            if (previous().type == ETokenType::SEMICOLON) return;

            switch (peek().type) {
                case ETokenType::CLASS:
                case ETokenType::FUNCTION:
                case ETokenType::VAR:
                case ETokenType::FOR:
                case ETokenType::IF:
                case ETokenType::WHILE:
                case ETokenType::RETURN:
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