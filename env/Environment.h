
#pragma once
#include <unordered_map>
#include <string>
#include <memory>
#include "../token/Token.h"
#include "../type/Dynamic.h"

// 存储变量用的类
class Environment {
    // 存储当前作用域中的所有变量名: 值，如{x: 5, y: "Hello World"}
    std::unordered_map<std::string, Dynamic> values;
    // 外层环境指针，指向其外层作用域(一个{})。内层作用域可以访问外层作用域变量，但是外层访问不到内层的。内层找不到就去外层找
    std::shared_ptr<Environment> enclosing;

public:
    // 全局环境构造
    Environment() : enclosing(nullptr) {}

    // 嵌套环境构造
    explicit Environment(std::shared_ptr<Environment> enclosing)
        : enclosing(enclosing) {}

    // 在当前环境定义新变量
    void define(const std::string& name, const Dynamic& value) {
        values[name] = value;
    }

    // 从内向外查找变量
    Dynamic get(const Token& name) {
        auto it = values.find(name.lexeme);
        if (it != values.end()) {
            return it->second;
        }

        // 去外层环境找
        if (enclosing) {
            return enclosing->get(name);
        }

        throw std::runtime_error("未定义的变量" + name.lexeme + "'。");
    }

    // 从内向外查找赋值
    void assign(const Token& name, const Dynamic& value) {
        auto it = values.find(name.lexeme);
        if (it != values.end()) {
            values[name.lexeme] = value;
            return;
        }

        if (enclosing) {
            enclosing->assign(name, value);
            return;
        }

        throw std::runtime_error("未定义的变量" + name.lexeme + "'。");
    }

    // 获取外层环境
    std::shared_ptr<Environment> getEnclosing() const {
        return enclosing;
    }
};