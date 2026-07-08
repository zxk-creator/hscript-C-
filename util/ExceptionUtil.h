//
// Created by kkplay on 7/7/26.
//

#pragma once
#include <stdexcept>
#include <string>

namespace Exception {
    enum class OperatorKind
    {
        add,
        subtract,
        multiply,
        divide,
        compare
    };

    inline void throwUnsupportedOperator(OperatorKind op)
    {
        std::string opName;
        switch (op) {
        case OperatorKind::add:
            opName = "加法";
            break;
        case OperatorKind::subtract:
            opName = "减法";
            break;
        case OperatorKind::multiply:
            opName = "乘法";
            break;
        case OperatorKind::divide:
            opName = "除法";
            break;
        case OperatorKind::compare:
            opName = "比较";
            break;
        default:
            opName = "未知运算符";
            break;
        }
        throw std::runtime_error("不支持的运算符操作: " + opName);
    }

    inline void throwDivideZero()
    {
        throw std::runtime_error("除数不能为零！");
    }

    inline void throwBadKind(std::string kind)
    {
        throw std::runtime_error("不是类型: " + kind);
    }

    inline void throwLogicalTypeError(const std::string& operation)
    {
        throw std::runtime_error("逻辑运算错误: " + operation + " 只能用于布尔值");
    }

    inline void throwComparisonTypeError(const std::string& operation)
    {
        throw std::runtime_error("比较运算错误: " + operation + " 不支持当前类型");
    }

    inline void throwConversionError(const std::string& from, const std::string& to)
    {
        throw std::runtime_error("类型转换错误: 无法从 " + from + " 转换为 " + to);
    }

    inline void throwRuntimeError(const std::string& msg)
    {
        throw std::runtime_error(msg);
    }
}