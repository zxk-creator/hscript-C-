// interpreter/Interpreter.h
#pragma once
#include <iostream>
#include <vector>
#include <memory>
#include "../ast/Expr.h"
#include "../env/Environment.h"
#include "../type/Dynamic.h"
#include "../util/ExceptionUtil.h"
#include "../type/HClass.h"
#include "../util/reflect/Reflect.h"

// 用于return语句的控制流异常
class ReturnException : public std::runtime_error {
    Dynamic _value;
public:
    ReturnException(const Dynamic& value)
        : std::runtime_error("return"), _value(value) {}
    Dynamic getValue() const { return _value; }
};

class Interpreter : public ExprVisitor, public StmtVisitor {

    std::shared_ptr<Environment> environment = std::make_shared<Environment>();
    // 可继承的类(由C++定义和暴露)，类型为 {类名: {方法名: 所有方法}}
    std::unordered_map<std::string, std::unordered_map<std::string,FunctionType>> inheritableNativeClasses;

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
    void executeBlock(const std::vector<std::unique_ptr<Stmt>>& statements, std::shared_ptr<Environment> env) {
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

    // 注册C++函数到全局环境
    void registerNativeFunc(const std::string& name, FunctionType func) {
        environment->define(name, Dynamic(std::move(func)));
    }

    /**
     * 注册新的可继承的C++类到全局
     * @param clsName C++类名
     * @param clsMethods 所包含的全部或部分方法
     */
    void registerNativeClass(const std::string& clsName, const std::unordered_map<std::string, FunctionType>& clsMethods)
    {
        inheritableNativeClasses[clsName] = clsMethods;
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

        if (expr.op.type == ETokenType::OR) {
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
            case ETokenType::BANG:
                // 逻辑非：真变假，假变真
                return Dynamic(!isTruthy(right));
            case ETokenType::MINUS:
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
            case ETokenType::PLUS:
                return left + right;
            case ETokenType::MINUS:
                return left - right;
            case ETokenType::STAR:
                return left * right;
            case ETokenType::SLASH:
                return left / right;

            // 比较运算符
            case ETokenType::GREATER:
                return Dynamic(left > right);
            case ETokenType::GREATER_EQUAL:
                return Dynamic(left >= right);
            case ETokenType::LESS:
                return Dynamic(left < right);
            case ETokenType::LESS_EQUAL:
                return Dynamic(left <= right);

            // 相等运算符
            case ETokenType::EQUAL_EQUAL:
                return Dynamic(left == right);
            case ETokenType::BANG_EQUAL:
                return Dynamic(left != right);

            default:
                Exception::throwRuntimeError("未知的二元运算符！");
        }
        return Dynamic();
    }

    // 从环境中获取值
    Dynamic visitVariableExpr(const Variable& expr) override {
        // 先查环境（局部变量、参数）
        try {
            return environment->get(expr.name);
        } catch (const std::runtime_error&) {
            // 如果在环境中找不到，尝试作为this的方法调用
            // 检查当前作用域是否有this
            try {
                Token thisToken(ETokenType::THIS, "this", "", 0);
                Dynamic thisVal = environment->get(thisToken);
                auto instance = std::dynamic_pointer_cast<HInstance>(thisVal.asObject());
                if (instance) {
                    // 先尝试执行脚本方法
                    if (const FunctionStmt* scriptMethod = instance->klass->findMethod(expr.name.lexeme).first) {
                        return createBoundMethod(instance, scriptMethod, instance->klass);
                    }
                    // 再尝试执行C++方法
                    if (const FunctionType nativeMethod = instance->klass->findMethod(expr.name.lexeme).second)
                    {
                        // 如果有效，直接执行
                       return Dynamic(nativeMethod);
                    }
                }
            } catch (...) {
                // 没有this上下文，忽略
            }
            // 重新抛出原始错误
            throw std::runtime_error("未定义的变量'" + expr.name.lexeme + "'。");
        }
    }

    // 计算新值并更新环境
    Dynamic visitAssignExpr(const Assign& expr) override {
        Dynamic value = evaluate(*expr.value);
        environment->assign(expr.name, value);
        return value;
    }

    // 函数调用: 计算callee和参数,然后调用
    Dynamic visitCallExpr(const Call& expr) override {
        Dynamic callee = evaluate(*expr.callee);

        std::vector<Dynamic> arguments;
        for (const auto& arg : expr.arguments) {
            arguments.push_back(evaluate(*arg));
        }

        if (!callee.isFunction()) {
            throw std::runtime_error("只能调用函数类型的值。");
        }

        return callee.call(arguments);
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

    // 函数声明：创建一个可调用的函数值并绑定到变量名
    Dynamic visitFunctionStmt(const FunctionStmt& stmt) override {
        // 创建闭包：捕获当前环境和AST节点
        auto func = [declaration = &stmt, closure = environment, this](const std::vector<Dynamic>& args) mutable -> Dynamic {
            // 创建函数调用时的新环境（闭包环境作为外层）
            auto env = std::make_shared<Environment>(closure);

            // 绑定参数
            for (size_t i = 0; i < declaration->params.size(); ++i) {
                env->define(declaration->params[i].lexeme, args[i]);
            }

            // 执行函数体
            try {
                executeBlock(declaration->body, env);
            } catch (ReturnException& e) {
                return e.getValue();  // return语句从这里捕获
            }

            return Dynamic();  // 默认返回null
        };

        // 将函数绑定到变量名
        environment->define(stmt.name.lexeme, Dynamic(FunctionType(func)));
        return Dynamic();
    }

    // return语句：抛出异常传递返回值
    Dynamic visitReturnStmt(const ReturnStmt& stmt) override {
        Dynamic value;
        if (stmt.value) {
            value = evaluate(*stmt.value);
        }
        throw ReturnException(value);
    }

    // ===== 类/对象相关 =====
    /**
     * 创建绑定方法（捕获实例用于this，捕获类用于super）
     * 所谓的this，实际上就是一个环境，找在这个环境里面的变量
     * @param instance 方法被调用时，this指向誰？
     * @param method 要执行什么方法？
     * @param definingClass 这个方法是从哪个类上找到的？
     * @return 可执行方法对象
     */
    Dynamic createBoundMethod(std::shared_ptr<HInstance> instance,
                              const FunctionStmt* method,
                              std::shared_ptr<HClass> definingClass) {
        return Dynamic([instance, method, definingClass, this](const std::vector<Dynamic>& args) mutable -> Dynamic {
            auto env = std::make_shared<Environment>(definingClass->closure);
            env->define("this", Dynamic(instance));
            // 存储定义类，供super解析（实际上这也是个变量，理论上你可以把他的父类改了，super是脱糖的结果）
            env->define("__class__", Dynamic(definingClass));

            for (size_t i = 0; i < method->params.size(); ++i) {
                env->define(method->params[i].lexeme, args[i]);
            }

            try {
                executeBlock(method->body, env);
            } catch (ReturnException& e) {
                return e.getValue();
            }
            return Dynamic();
        });
    }

    // 类声明：创建HClass并绑定到变量名
    Dynamic visitClassStmt(const ClassStmt& stmt) override {
        // 解析父类
        std::shared_ptr<HClass> superclassObj = nullptr;
        if (stmt.superclass) {
            Dynamic superVal = evaluate(*stmt.superclass);
            auto superPtr = std::dynamic_pointer_cast<HClass>(superVal.asObject());
            if (!superPtr) {
                throw std::runtime_error("父类必须是类类型。");
            }
            superclassObj = superPtr;
        }

        // 创建类对象
        auto klass = std::make_shared<HClass>(stmt.name.lexeme, superclassObj);
        klass->closure = environment;  // 捕获当前环境作为闭包

        // 将方法加入类（raw pointer，所有权在AST）
        for (const auto& method : stmt.methods) {
            klass->methods[method->name.lexeme] = method.get();
        }

        // 绑定到变量名
        environment->define(stmt.name.lexeme, Dynamic(std::move(klass)));
        return Dynamic();
    }

    // 属性/方法访问: obj.field 或 obj.method
    Dynamic visitGetExpr(const Get& expr) override {
        // 处理 super.method()前面的super.必须率先检查，不能先求值super表达式
        if (dynamic_cast<const Super*>(&*expr.object)) {
            Token thisToken(ETokenType::THIS, "this", "", 0);
            Dynamic thisVal = environment->get(thisToken);
            auto instance = std::dynamic_pointer_cast<HInstance>(thisVal.asObject());
            if (!instance) {
                throw std::runtime_error("super只能在类方法中使用。");
            }

            // 获取当前类上下文
            Token classToken(ETokenType::IDENTIFIER, "__class__", "", 0);
            Dynamic classVal = environment->get(classToken);
            auto currentClass = std::dynamic_pointer_cast<HClass>(classVal.asObject());

            if (!currentClass || !currentClass->superclass) {
                throw std::runtime_error("该类没有父类，不能使用super。");
            }

            // 先尝试执行脚本父类方法
            if (const FunctionStmt* method = currentClass->superclass->findMethod(expr.name.lexeme).first) {
                return createBoundMethod(instance, method, currentClass->superclass);
            }
            // 如果不行？说明可能是C++父类，再尝试执行C++父类方法
            if (const FunctionType nativeMethod = currentClass->superclass->findMethod(expr.name.lexeme).second)
            {
                return Dynamic(nativeMethod);
            }
            throw std::runtime_error("父类中未找到方法 '" + expr.name.lexeme + "'。");

        }

        // 如果不是super，则处理实例属性/方法（常规对象访问时才求值）
        Dynamic object = evaluate(*expr.object);
        if (object.isNull()) {
            throw std::runtime_error("不能访问null对象的属性。");
        }

        auto instance = std::dynamic_pointer_cast<HInstance>(object.asObject());
        if (!instance) {
            throw std::runtime_error("只有对象实例有属性。");
        }

        // 先查找脚本字段字段（未定义的字段返回null）
        auto fieldIt = instance->fields.find(expr.name.lexeme);
        if (fieldIt != instance->fields.end()) {
            return fieldIt->second;
        }
        // 脚本字段没有？可能是C++字段
        auto klassPtr = instance->klass;
        // 反射调用
        if (klassPtr){
            return klassPtr->getField(expr.name.lexeme);
        }

        // 跑到这里说明上面的不是字段，那么我们查找方法，先脚本
        if (const FunctionStmt* method = instance->klass->findMethod(expr.name.lexeme).first) {
            return createBoundMethod(instance, method, instance->klass);
        }
        // 失败了，再C++方法试试？
        if (const FunctionType nativeMethod = instance->klass->findMethod(expr.name.lexeme).second)
        {
            return Dynamic(nativeMethod);
        }

        // 都没有，直接抛异常
        throw std::runtime_error("对象中未找到属性 '" + expr.name.lexeme + "'。");
    }

    // 属性赋值: obj.field = value
    Dynamic visitSetExpr(const Set& expr) override {
        Dynamic object = evaluate(*expr.object);

        if (object.isNull()) {
            throw std::runtime_error("不能设置null对象的属性。");
        }

        auto instance = std::dynamic_pointer_cast<HInstance>(object.asObject());
        if (!instance) {
            throw std::runtime_error("只有对象实例可以设置属性。");
        }


        Dynamic value = evaluate(*expr.value);
        // 先找脚本是否真的有这个字段
        if (instance->fields.find(expr.name.lexeme) != instance->fields.end())
        {
            instance->fields[expr.name.lexeme] = value;
            return value;
        }
        // 再找C++是否有这个字段
        if (instance->klass && instance->klass->hasField(expr.name.lexeme))
        {
            instance->klass->setField(expr.name.lexeme, value);
            return value;
        }

        // 都没，异常
        throw std::runtime_error("对象中未找到属性 '" + expr.name.lexeme + "'。");
    }

    // this关键字：从环境中查找
    Dynamic visitThisExpr(const This& expr) override {
        Token thisToken(ETokenType::THIS, "this", "", 0);
        return environment->get(thisToken);
    }

    // super关键字不能单独使用
    Dynamic visitSuperExpr(const Super& expr) override {
        throw std::runtime_error("super不能单独使用，必须通过super.method()调用方法。");
    }

    // new表达式: new ClassName(args)
    // 这里是关键: 如果我们创建一个脚本类，他继承自C++类怎么办？我这里的解决方案是：创建一个脚本类，再创建一个C++父类。返回的是HClass基类指针。
    Dynamic visitNewExpr(const NewExpr& expr) override {
        // 查找类
        Dynamic classVal = environment->get(expr.name);
        auto klass = std::dynamic_pointer_cast<HClass>(classVal.asObject());
        // 如果找到父类是一个脚本类
        if (klass) {
            // 创建实例
            auto instance = std::make_shared<HInstance>(klass);

            // 计算参数
            std::vector<Dynamic> args;
            for (const auto& arg : expr.arguments) {
                args.push_back(evaluate(*arg));
            }

            // 调用构造函数（如果存在）
            const FunctionStmt* constructor = klass->findMethod("new").first;
            if (constructor) {
                auto boundCtor = createBoundMethod(instance, constructor, klass);
                boundCtor.call(args);
            }

            return Dynamic(instance);
        }
        // 如果没找到，那说不定是C++父类？所以我们取出类名
        auto it = inheritableNativeClasses.find(expr.name.literal);
        // 找到！直接实例化
        if (it != inheritableNativeClasses.end())
        {
            std::vector<Dynamic> args;
            for (const auto& arg : expr.arguments) {
                args.push_back(evaluate(*arg));
            }
            // 如果指针有效，说明创建实例成功！
            if(auto ptr = Reflect::createInstance(expr.name.literal,args))
            {
                // 创建一个空脚本对象包装一下！
                auto insPtr = std::make_shared<HInstance>(ptr);
                return Dynamic(insPtr);
            }
        }
        throw std::runtime_error("未找到类 '" + expr.name.lexeme + "'。");
    }

};