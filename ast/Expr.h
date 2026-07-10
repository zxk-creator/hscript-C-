//
// AST节点定义
//

// Expr.h
#pragma once
#include <memory>
#include "../token/Token.h"
#include "../type/Dynamic.h"

class Variable;
class Assign;
class Call;
class Expr;
class Binary;
class Grouping;
class Literal;
class Unary;
class Logical;
class Get;
class Set;
class This;
class Super;
class NewExpr;

class FunctionStmt;
class ReturnStmt;
class ClassStmt;

class ExprVisitor {
public:
    virtual Dynamic visitBinaryExpr(const Binary& expr) = 0;
    virtual Dynamic visitGroupingExpr(const Grouping& expr) = 0;
    virtual Dynamic visitLiteralExpr(const Literal& expr) = 0;
    virtual Dynamic visitUnaryExpr(const Unary& expr) = 0;
    virtual Dynamic visitVariableExpr(const Variable& expr) = 0;
    virtual Dynamic visitAssignExpr(const Assign& expr) = 0;
    virtual Dynamic visitLogicalExpr(const Logical& expr) = 0;
    virtual Dynamic visitCallExpr(const Call& expr) = 0;
    virtual Dynamic visitGetExpr(const Get& expr) = 0;
    virtual Dynamic visitSetExpr(const Set& expr) = 0;
    virtual Dynamic visitThisExpr(const This& expr) = 0;
    virtual Dynamic visitSuperExpr(const Super& expr) = 0;
    virtual Dynamic visitNewExpr(const NewExpr& expr) = 0;
    virtual ~ExprVisitor() = default;
};

// 用于AST的每一个节点
class Expr {
public:
    virtual ~Expr() = default;

    virtual Dynamic accept(ExprVisitor& visitor) const = 0;
};

// 二元运算节点
class Binary : public Expr {
public:
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;

    Binary(std::unique_ptr<Expr> left, Token& op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitBinaryExpr(*this);
    }
};

// 括号分组
class Grouping : public Expr {
public:
    std::unique_ptr<Expr> expression;

    Grouping(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitGroupingExpr(*this);
    }

};

// && 和 ||运算符
class Logical : public Expr {
public:
    std::unique_ptr<Expr> left;
    Token op;
    std::unique_ptr<Expr> right;

    Logical(std::unique_ptr<Expr> left, const Token& op, std::unique_ptr<Expr> right)
        : left(std::move(left)), op(op), right(std::move(right)) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitLogicalExpr(*this);
    }
};



// 函数调用表达式: callee(args)
class Call : public Expr {
public:
    std::unique_ptr<Expr> callee;
    Token paren;
    std::vector<std::unique_ptr<Expr>> arguments;

    Call(std::unique_ptr<Expr> callee, Token paren, std::vector<std::unique_ptr<Expr>> arguments)
        : callee(std::move(callee)), paren(paren), arguments(std::move(arguments)) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitCallExpr(*this);
    }
};

// 属性访问: object.name
class Get : public Expr {
public:
    std::unique_ptr<Expr> object;
    Token name;

    Get(std::unique_ptr<Expr> object, Token name)
        : object(std::move(object)), name(name) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitGetExpr(*this);
    }
};

// 属性赋值: object.name = value
class Set : public Expr {
public:
    std::unique_ptr<Expr> object;
    Token name;
    std::unique_ptr<Expr> value;

    Set(std::unique_ptr<Expr> object, Token name, std::unique_ptr<Expr> value)
        : object(std::move(object)), name(name), value(std::move(value)) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitSetExpr(*this);
    }
};

// this关键字
class This : public Expr {
public:
    Token keyword;

    explicit This(Token keyword) : keyword(keyword) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitThisExpr(*this);
    }
};

// super关键字（仅用于 super.method()）
class Super : public Expr {
public:
    Token keyword;

    explicit Super(Token keyword) : keyword(keyword) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitSuperExpr(*this);
    }
};

// new 表达式: new ClassName(args)
class NewExpr : public Expr {
public:
    Token name;  // 类名
    std::vector<std::unique_ptr<Expr>> arguments;

    NewExpr(Token name, std::vector<std::unique_ptr<Expr>> arguments)
        : name(name), arguments(std::move(arguments)) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitNewExpr(*this);
    }
};

// ==================================================关键字AST===============================================================
class Stmt;
class ExpressionStmt;
class VarStmt;
class BlockStmt;
class IfStmt;
class WhileStmt;

class StmtVisitor {
public:
    virtual Dynamic visitExpressionStmt(const ExpressionStmt& stmt) = 0;
    virtual Dynamic visitVarStmt(const VarStmt& stmt) = 0;
    virtual Dynamic visitBlockStmt(const BlockStmt& stmt) = 0;
    virtual Dynamic visitIfStmt(const IfStmt& stmt) = 0;
    virtual Dynamic visitWhileStmt(const WhileStmt& stmt) = 0;
    virtual Dynamic visitFunctionStmt(const FunctionStmt& stmt) = 0;
    virtual Dynamic visitReturnStmt(const ReturnStmt& stmt) = 0;
    virtual Dynamic visitClassStmt(const ClassStmt& stmt) = 0;
    virtual ~StmtVisitor() = default;
};

// 作为语句(var等关键字)AST的节点
class Stmt {
public:
    virtual ~Stmt() = default;
    virtual Dynamic accept(StmtVisitor& visitor) const = 0;
};

// 表达式语句
class ExpressionStmt : public Stmt {
public:
    // 例如：1 + 2这样的表达式
    std::unique_ptr<Expr> expression;

    explicit ExpressionStmt(std::unique_ptr<Expr> expression)
        : expression(std::move(expression)) {}

    Dynamic accept(StmtVisitor& visitor) const override {
        return visitor.visitExpressionStmt(*this);
    }
};

// var
class VarStmt : public Stmt {
public:
    Token name;
    // var后面的初始化表达式
    std::unique_ptr<Expr> initializer;

    VarStmt(const Token& name, std::unique_ptr<Expr> initializer)
        : name(name), initializer(std::move(initializer)) {}

    Dynamic accept(StmtVisitor& visitor) const override {
        return visitor.visitVarStmt(*this);
    }
};

// {}
class BlockStmt : public Stmt {
public:
    // 括号内的语句
    std::vector<std::unique_ptr<Stmt>> statements;

    explicit BlockStmt(std::vector<std::unique_ptr<Stmt>> statements)
        : statements(std::move(statements)) {}

    Dynamic accept(StmtVisitor& visitor) const override {
        return visitor.visitBlockStmt(*this);
    }
};

// 字面量
class Literal : public Expr {
public:
    Dynamic value;

    Literal(Dynamic value) : value(std::move(value)) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitLiteralExpr(*this);
    }
};

// 一元运算
class Unary : public Expr {
public:
    Token op;
    std::unique_ptr<Expr> right;

    Unary(Token op, std::unique_ptr<Expr> right)
        : op(op), right(std::move(right)) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitUnaryExpr(*this);
    }
};

// 读取变量
class Variable : public Expr {
public:
    Token name;

    explicit Variable(const Token& name) : name(name) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitVariableExpr(*this);
    }
};

// 赋值操作
class Assign : public Expr {
public:
    Token name;
    std::unique_ptr<Expr> value;

    Assign(const Token& name, std::unique_ptr<Expr> value)
        : name(name), value(std::move(value)) {}

    Dynamic accept(ExprVisitor& visitor) const override {
        return visitor.visitAssignExpr(*this);
    }
};

// 关键字if
class IfStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> thenBranch;
    std::unique_ptr<Stmt> elseBranch;  // 可为 nullptr

    IfStmt(std::unique_ptr<Expr> condition,
           std::unique_ptr<Stmt> thenBranch,
           std::unique_ptr<Stmt> elseBranch)
        : condition(std::move(condition))
        , thenBranch(std::move(thenBranch))
        , elseBranch(std::move(elseBranch)) {}

    Dynamic accept(StmtVisitor& visitor) const override {
        return visitor.visitIfStmt(*this);
    }
};

// 关键字while
class WhileStmt : public Stmt {
public:
    std::unique_ptr<Expr> condition;
    std::unique_ptr<Stmt> body;

    WhileStmt(std::unique_ptr<Expr> condition, std::unique_ptr<Stmt> body)
        : condition(std::move(condition)), body(std::move(body)) {}

    Dynamic accept(StmtVisitor& visitor) const override {
        return visitor.visitWhileStmt(*this);
    }
};

// 脚本函数
class FunctionStmt : public Stmt {
public:
    Token name;
    std::vector<Token> params;
    std::vector<std::unique_ptr<Stmt>> body;

    FunctionStmt(const Token& name, std::vector<Token> params, std::vector<std::unique_ptr<Stmt>> body)
        : name(name), params(std::move(params)), body(std::move(body)) {}

    Dynamic accept(StmtVisitor& visitor) const override {
        return visitor.visitFunctionStmt(*this);
    }
};

// return语句: return expression?;
class ReturnStmt : public Stmt {
public:
    Token keyword;
    std::unique_ptr<Expr> value;

    ReturnStmt(const Token& keyword, std::unique_ptr<Expr> value)
        : keyword(keyword), value(std::move(value)) {}

    Dynamic accept(StmtVisitor& visitor) const override {
        return visitor.visitReturnStmt(*this);
    }
};

// 类声明: class name extends superclass { ... }
class ClassStmt : public Stmt {
public:
    Token name;
    std::unique_ptr<Expr> superclass;  // Variable 或 nullptr
    std::vector<std::unique_ptr<FunctionStmt>> methods;
    // 字段声明: {字段名, 初始化表达式}，无初始化式时 second == nullptr
    std::vector<std::pair<Token, std::unique_ptr<Expr>>> fields;

    ClassStmt(Token name, std::unique_ptr<Expr> superclass,
              std::vector<std::unique_ptr<FunctionStmt>> methods,
              std::vector<std::pair<Token, std::unique_ptr<Expr>>> fields)
        : name(name), superclass(std::move(superclass)), methods(std::move(methods)), fields(std::move(fields)) {}

    Dynamic accept(StmtVisitor& visitor) const override {
        return visitor.visitClassStmt(*this);
    }
};
