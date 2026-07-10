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
                execute(*stmt);
            }
            // 异常时恢复环境
        } catch (...) {
            environment = previous;
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

    /**
     * 注册C++函数到全局环境
     * @param name 函数名（不一定和C++这边的一样）
     * @param func C++函数lambda
     */
    void registerNativeFunc(const std::string& name, FunctionType func) {
        environment->define(name, Dynamic(std::move(func)));
    }

    /**
     * 注册新的可继承的C++类到全局
     * 注意：这不会把类名注册到脚本环境中（否则 new TestClass() 会误走脚本实例化路径）
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

    // 执行逻辑运算符
    Dynamic visitLogicalExpr(const Logical& expr) override {
        Dynamic left = evaluate(*expr.left);

        if (expr.op.type == ETokenType::OR) {
            if (isTruthy(left)) return left;
        } else {
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
                // 逻辑非
                return Dynamic(!isTruthy(right));
            case ETokenType::MINUS:
                // 负号
                if (!right.isNumber()) {
                    throw std::runtime_error("操作数必须是数字。");
                }
                return Dynamic(-right.asNumber());
            default:
                Exception::throwRuntimeError("未知的一元运算符！");
        }
        return Dynamic();
    }

    Dynamic visitBinaryExpr(const Binary& expr) override {
        Dynamic left = evaluate(*expr.left);
        Dynamic right = evaluate(*expr.right);

        switch (expr.op.type) {

            case ETokenType::PLUS:
                return left + right;
            case ETokenType::MINUS:
                return left - right;
            case ETokenType::STAR:
                return left * right;
            case ETokenType::SLASH:
                return left / right;

            case ETokenType::GREATER:
                return Dynamic(left > right);
            case ETokenType::GREATER_EQUAL:
                return Dynamic(left >= right);
            case ETokenType::LESS:
                return Dynamic(left < right);
            case ETokenType::LESS_EQUAL:
                return Dynamic(left <= right);

            case ETokenType::EQUAL_EQUAL:
                return Dynamic(left == right);
            case ETokenType::BANG_EQUAL:
                return Dynamic(left != right);

            default:
                Exception::throwRuntimeError("未知的二元运算符，这不应该发生，说明是C++解释器出了问题！");
        }
        return Dynamic();
    }

    // 从环境中获取值
    Dynamic visitVariableExpr(const Variable& expr) override {
        // 局部变量、参数
        try {
            return environment->get(expr.name);
        } catch (const std::runtime_error&) {
            // 环境中找不到尝试作为this的字段或方法访问
            try {
                Token thisToken(ETokenType::THIS, "this", "", 0);
                Dynamic thisVal = environment->get(thisToken);
                auto instance = std::dynamic_pointer_cast<HInstance>(thisVal.asObject());
                if (instance) {
                    // 先查实例字段
                    auto fieldIt = instance->fields.find(expr.name.lexeme);
                    if (fieldIt != instance->fields.end()) {
                        return fieldIt->second;
                    }
                    // 找不到？再查C++字段
                    if (instance->klass && instance->klass->hasField(expr.name.lexeme)) {
                        return instance->klass->getField(expr.name.lexeme);
                    }
                    // 说不定是方法？所以查脚本方法
                    if (const FunctionStmt* scriptMethod = instance->klass->findMethod(expr.name.lexeme).first) {
                        return createBoundMethod(instance, scriptMethod, instance->klass);
                    }
                    // 最后保底查C++方法
                    if (const FunctionType nativeMethod = instance->klass->findMethod(expr.name.lexeme).second) {
                        return Dynamic(nativeMethod);
                    }
                }
            } catch (...) {
                // 没有this上下文，忽略
            }
            // 都找不到，抛异常
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
    // 执行表达式并丢弃结果
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
        // 没有初始值则默认为null
        environment->define(stmt.name.lexeme, value);
        return Dynamic();
    }

    // 对于{}：创建新环境并执行块内语句
    Dynamic visitBlockStmt(const BlockStmt& stmt) override {
        auto newEnv = std::make_shared<Environment>(environment);
        executeBlock(stmt.statements, newEnv);
        return Dynamic();
    }

    // 函数声明：创建一个可调用的函数值并绑定到变量名
    Dynamic visitFunctionStmt(const FunctionStmt& stmt) override {
        // 创建闭包：捕获当前环境和AST节点
        auto func = [declaration = &stmt, closure = environment, this](const std::vector<Dynamic>& args) mutable -> Dynamic {
            // 参数数量校验
            if (args.size() != declaration->params.size()) {
                throw std::runtime_error("函数 '" + declaration->name.lexeme + "' 参数数量不匹配，期望 "
                    + std::to_string(declaration->params.size()) + " 个，但你传入了" + std::to_string(args.size()) + " 个。");
            }

            // 创建函数调用时的新环境
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
            // 参数数量校验
            if (args.size() != method->params.size()) {
                throw std::runtime_error("方法 '" + method->name.lexeme + "' 参数数量不匹配，期望 "
                    + std::to_string(method->params.size()) + " 个，传入 " + std::to_string(args.size()) + " 个。");
            }

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
            try {
                Dynamic superVal = evaluate(*stmt.superclass);
                superclassObj = std::dynamic_pointer_cast<HClass>(superVal.asObject());
            } catch (const std::runtime_error&) {
                // 脚本环境中没找到，可能是C++类
                if (auto* varExpr = dynamic_cast<Variable*>(stmt.superclass.get())) {
                    auto it = inheritableNativeClasses.find(varExpr->name.lexeme);
                    if (it != inheritableNativeClasses.end()) {
                        std::vector<Dynamic> dummyArgs;
                        superclassObj = Reflect::createInstance(varExpr->name.lexeme, dummyArgs);
                    }
                }
            }
            if (!superclassObj) {
                throw std::runtime_error("父类必须是类类型。");
            }
        }

        // 创建类对象
        auto klass = std::make_shared<HClass>(stmt.name.lexeme, superclassObj);
        klass->closure = environment;  // 捕获当前环境作为闭包

        // 将方法加入类
        for (const auto& method : stmt.methods) {
            klass->methods[method->name.lexeme] = method.get();
        }

        // 将字段声明加入类，用于实例化时创建字段并赋初值
        for (const auto& field : stmt.fields) {
            klass->fieldDecls.push_back({field.first.lexeme, field.second.get()});
        }

        // 绑定到变量名
        environment->define(stmt.name.lexeme, Dynamic(std::move(klass)));
        return Dynamic();
    }

    // 属性/方法访问: obj.field或obj.method
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

        // 如果不是super，则处理实例属性或方法
        Dynamic object = evaluate(*expr.object);
        if (object.isNull()) {
            throw std::runtime_error("不能访问null对象的属性。");
        }
        if (!object.isObject()) {
            // 基本类型方法支持，这是可以对数字可以调用toString()的原因！
            if (expr.name.lexeme == "toString" || expr.name.lexeme == "转字符串") {
                return Dynamic([object](const std::vector<Dynamic>&) -> Dynamic {
                    return Dynamic(object.toString());
                });
            }
            throw std::runtime_error("只有对象实例有属性。");
        }

        auto instance = std::dynamic_pointer_cast<HInstance>(object.asObject());
        if (!instance) {
            throw std::runtime_error("只有对象实例有属性。");
        }

        // 先查找脚本字段
        auto fieldIt = instance->fields.find(expr.name.lexeme);
        if (fieldIt != instance->fields.end()) {
            return fieldIt->second;
        }

        // 查找方法,先脚本后C++,必须放在 getField 之前，因为方法名不是字段
        if (const FunctionStmt* method = instance->klass->findMethod(expr.name.lexeme).first) {
            return createBoundMethod(instance, method, instance->klass);
        }
        if (const FunctionType nativeMethod = instance->klass->findMethod(expr.name.lexeme).second)
        {
            return Dynamic(nativeMethod);
        }

        // 再尝试反射到C++字段
        if (instance->klass && instance->klass->hasField(expr.name.lexeme)) {
            return instance->klass->getField(expr.name.lexeme);
        }

        // 都没有，直接抛异常
        throw std::runtime_error("对象中未找到属性 '" + expr.name.lexeme + "'。");
    }

    // obj.field = value
    Dynamic visitSetExpr(const Set& expr) override {
        Dynamic object = evaluate(*expr.object);

        if (object.isNull()) {
            throw std::runtime_error("不能设置null对象的属性。");
        }
        if (!object.isObject()) {
            throw std::runtime_error("只有对象实例可以设置属性。");
        }

        auto instance = std::dynamic_pointer_cast<HInstance>(object.asObject());
        if (!instance) {
            throw std::runtime_error("只有对象实例可以设置属性。");
        }


        Dynamic value = evaluate(*expr.value);

        // 字段已经在脚本层存在直接更新
        if (instance->fields.find(expr.name.lexeme) != instance->fields.end())
        {
            instance->fields[expr.name.lexeme] = value;
            return value;
        }

        // 字段在C++层存在通过C++ setter
        if (instance->klass && instance->klass->hasField(expr.name.lexeme))
        {
            instance->klass->setField(expr.name.lexeme, value);
            return value;
        }

        // 都没有自动在脚本层创建新字段
        instance->fields[expr.name.lexeme] = value;
        return value;
    }

    // this关键字从环境中查找
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
        // 查找类 - 可能是脚本类或C++类
        std::shared_ptr<HClass> klass = nullptr;
        try {
            Dynamic classVal = environment->get(expr.name);
            klass = std::dynamic_pointer_cast<HClass>(classVal.asObject());
        } catch (const std::runtime_error&) {
            // 未找到脚本类，可能是C++类
            klass = nullptr;
        }

        // 脚本类路径（包括脚本类继承脚本类、脚本类继承C++类）
        if (klass) {
            // 创建实例
            auto instance = std::make_shared<HInstance>(klass);

            // 计算参数
            std::vector<Dynamic> args;
            for (const auto& arg : expr.arguments) {
                args.push_back(evaluate(*arg));
            }

            // 先执行字段初始值（在构造函数之前）
            for (const auto& field : klass->fieldDecls) {
                if (field.initializer) {
                    instance->fields[field.name] = evaluate(*field.initializer);
                } else {
                    instance->fields[field.name] = Dynamic();  // null
                }
            }

            // 调用构造函数
            const FunctionStmt* constructor = klass->findMethod("new").first;
            if (constructor) {
                auto boundCtor = createBoundMethod(instance, constructor, klass);
                boundCtor.call(args);
            }
            // 可能是中文的？于是我们也查找中文方法
            const FunctionStmt* constructorC = klass->findMethod("新的").first;
            if (constructorC) {
                auto boundCtor = createBoundMethod(instance, constructorC, klass);
                boundCtor.call(args);
            }

            // 如果没有构造函数，也会构造这个类。
            return Dynamic(instance);
        }
        // 如果没找到，那说不定是C++父类？所以我们取出类名
        auto it = inheritableNativeClasses.find(expr.name.lexeme);
        // 找到直接实例化
        if (it != inheritableNativeClasses.end())
        {
            std::vector<Dynamic> args;
            for (const auto& arg : expr.arguments) {
                args.push_back(evaluate(*arg));
            }
            // 通过反射创建C++对象实例
            if(auto ptr = Reflect::createInstance(expr.name.lexeme, args))
            {
                // 创建一个HInstance包裹C++对象，instance->klass指向C++对象本身
                auto insPtr = std::make_shared<HInstance>(ptr);
                return Dynamic(insPtr);
            }
        }
        throw std::runtime_error("未找到类 '" + expr.name.lexeme + "'。");
    }

};