// interpreter/Interpreter.h
#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include "../ast/Expr.h"
#include "../env/Environment.h"
#include "../type/Dynamic.h"
#include "../util/ExceptionUtil.h"

class Interpreter : public ExprVisitor, public StmtVisitor {
    std::shared_ptr<Environment> environment = std::make_shared<Environment>();

    // 判断值的真假
    bool isTruthy(const Dynamic& value) {
        if (value.isNull()) return false;
        if (value.isBool()) return value.asBool();
        return true;  // 数字、字符串等非空值都是真
    }

    // 将值转换为字符串
    std::string stringify(const Dynamic& value) {
        return value.toString();
    }

    // 在新建的环境中执行语句块
    void executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements,
                      std::shared_ptr<Environment> env) {
        auto previous = environment;
        try {
            environment = env;  // 切换到新环境
            for (const auto& stmt : statements) {
                execute(*stmt);  // 逐条执行
            }
        } catch (...) {
            environment = previous;  // 异常时恢复环境
            throw;
        }
        environment = previous;  // 执行完恢复环境
    }

public:
    // 求值表达式
    Dynamic evaluate(const Expr& expr) {
        return expr.accept(*this);
    }

    // 执行语句
    void execute(const Stmt& stmt) {
        stmt.accept(*this);
    }

    // 解释执行整个程序
    void interpret(const std::vector<std::unique_ptr<Stmt>>& statements) {
        for (const auto& stmt : statements) {
            execute(*stmt);
        }
    }

    // ===== ExprVisitor 实现 =====
    // 字面量直接返回存储的值
    Dynamic visitLiteralExpr(const Literal& expr) override {
        return expr.value;
    }

    Dynamic visitLogicalExpr(const Logical& expr) override {
        Dynamic left = evaluate(*expr.left);

        if (expr.op.type == TokenType::OR) {
            if (isTruthy(left)) return left;
        } else {  // AND
            if (!isTruthy(left)) return left;
        }

        return evaluate(*expr.right);
    }

    Dynamic visitIfStmt(const IfStmt& stmt) override {
        if (isTruthy(evaluate(*stmt.condition))) {
            execute(*stmt.thenBranch);
        } else if (stmt.elseBranch) {
            execute(*stmt.elseBranch);
        }
        return Dynamic();
    }

    Dynamic visitWhileStmt(const WhileStmt& stmt) override {
        while (isTruthy(evaluate(*stmt.condition))) {
            execute(*stmt.body);
        }
        return Dynamic();
    }

    // 递归计算内部表达式
    Dynamic visitGroupingExpr(const Grouping& expr) override {
        return evaluate(*expr.expression);
    }

    // ! 和 -
    Dynamic visitUnaryExpr(const Unary& expr) override {
        Dynamic right = evaluate(*expr.right);

        switch (expr.op.type) {
            case TokenType::BANG:
                // 逻辑非：真变假，假变真
                return Dynamic(!isTruthy(right));
            case TokenType::MINUS:
                // 负号：必须是数字
                if (!right.isNumber()) {
                    throw std::runtime_error("操作数必须是数字。");
                }
                return Dynamic(-right.asNumber());
            default:
                Exception::throwRuntimeError("未知的一元运算符！");
        }
        return Dynamic();
    }

    // 算术、比较、相等
    Dynamic visitBinaryExpr(const Binary& expr) override {
        Dynamic left = evaluate(*expr.left);
        Dynamic right = evaluate(*expr.right);

        switch (expr.op.type) {
            // 算术运算符
            case TokenType::PLUS:
                return left + right;
            case TokenType::MINUS:
                return left - right;
            case TokenType::STAR:
                return left * right;
            case TokenType::SLASH:
                return left / right;

            // 比较运算符
            case TokenType::GREATER:
                return Dynamic(left > right);
            case TokenType::GREATER_EQUAL:
                return Dynamic(left >= right);
            case TokenType::LESS:
                return Dynamic(left < right);
            case TokenType::LESS_EQUAL:
                return Dynamic(left <= right);

            // 相等运算符
            case TokenType::EQUAL_EQUAL:
                return Dynamic(left == right);
            case TokenType::BANG_EQUAL:
                return Dynamic(left != right);

            default:
                Exception::throwRuntimeError("未知的二元运算符！");
        }
        return Dynamic();
    }

    // 从环境中获取值
    Dynamic visitVariableExpr(const Variable& expr) override {
        return environment->get(expr.name);
    }

    // 计算新值并更新环境
    Dynamic visitAssignExpr(const Assign& expr) override {
        Dynamic value = evaluate(*expr.value);
        environment->assign(expr.name, value);
        return value;
    }

    // ===== StmtVisitor 实现 =====
    // 表达式语句：执行表达式并丢弃结果
    Dynamic visitExpressionStmt(const ExpressionStmt& stmt) override {
        evaluate(*stmt.expression);
        return Dynamic();
    }

    // 计算初始值并存入环境
    Dynamic visitVarStmt(const VarStmt& stmt) override {
        Dynamic value;
        if (stmt.initializer) {
            value = evaluate(*stmt.initializer);  // 有初始值就计算
        }
        // 没有初始值则默认为 null
        environment->define(stmt.name.lexeme, value);
        return Dynamic();
    }

    // 代码块：创建新环境并执行块内语句
    Dynamic visitBlockStmt(const BlockStmt& stmt) override {
        auto newEnv = std::make_shared<Environment>(environment);
        executeBlock(stmt.statements, newEnv);
        return Dynamic();
    }
};