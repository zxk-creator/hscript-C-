//
// Created by kkplay on 7/7/26.
//

#pragma once
#include <cmath>
#include <stdexcept>
#include <string>
#include <memory>
#include <vector>
#include <variant>
#include <functional>
#include "BaseClass.h"
#include "../util/ExceptionUtil.h"

using String = std::string;

class Dynamic;

using NullType = std::monostate;
using ArrayType = std::vector<Dynamic>;
using ObjectType = std::shared_ptr<Object>;
// 必须把参数全都转换成
using FunctionType = std::function<Dynamic(const std::vector<Dynamic>&)>;

using DynamicValue = std::variant<
    NullType,       // null
    bool,           // 布尔值
    double,         // 数值统一为double
    String,         // 字符串
    ArrayType,      // 数组
    ObjectType,     // 对象
    FunctionType    // 函数
>;

// 允许不同类的operator()能够互相兼容
template<class... Ts>
struct overloaded : Ts... {
    using Ts::operator()...;
};
// 给编译器看的，我不写尖括号了，直接写参数，应该被理解成什么样？
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

class Dynamic
{
    DynamicValue _value;

    // 将 Dynamic 转换为字符串的辅助函数
    static String valueToString(const DynamicValue& val) {
        return std::visit(overloaded{
            [](NullType) -> String { return "null"; },
            [](bool b) -> String { return b ? "true" : "false"; },
            [](double d) -> String {
                if (d == static_cast<int64_t>(d)) {
                    return std::to_string(static_cast<int64_t>(d));
                }
                return std::to_string(d);
            },
            [](const String& s) -> String { return s; },
            [](const ArrayType& arr) -> String {
                /**
                String result = "[";
                for (size_t i = 0; i < arr.size(); ++i) {
                    if (i > 0) result += ", ";
                    result += arr[i].toString();
                }
                return result + "]";
                **/
                return "这是一个数组";
            },
            [](const ObjectType&) -> String { return "[对象]"; },
            [](const FunctionType&) -> String { return "[函数]"; }
        }, val);
    }

public:
    // ============ 构造函数 ============
    Dynamic() : _value(NullType{}) {}

    // 数值构造
    Dynamic(int v) : _value(static_cast<double>(v)) {}
    Dynamic(long v) : _value(static_cast<double>(v)) {}
    Dynamic(long long v) : _value(static_cast<double>(v)) {}
    Dynamic(float v) : _value(static_cast<double>(v)) {}
    Dynamic(double v) : _value(v) {}

    // 布尔构造
    Dynamic(bool v) : _value(v) {}

    // 字符串构造（支持移动语义）
    Dynamic(const char* v) : _value(String(v)) {}
    Dynamic(String v) : _value(std::move(v)) {}

    // 对象构造
    Dynamic(ObjectType v) : _value(std::move(v)) {}

    // 数组构造
    Dynamic(std::initializer_list<Dynamic> list) : _value(ArrayType(list)) {}
    Dynamic(ArrayType arr) : _value(std::move(arr)) {}

    // 函数构造
    Dynamic(FunctionType func) : _value(std::move(func)) {}

    // nullptr 构造
    Dynamic(std::nullptr_t) : _value(NullType{}) {}

    // 拷贝/移动构造（默认即可）
    Dynamic(const Dynamic&) = default;
    Dynamic(Dynamic&&) noexcept = default;

    // ============ 赋值运算符 ============
    Dynamic& operator=(const Dynamic&) = default;
    Dynamic& operator=(Dynamic&&) noexcept = default;

    // 从基本类型赋值
    Dynamic& operator=(int v) { _value = static_cast<double>(v); return *this; }
    Dynamic& operator=(double v) { _value = v; return *this; }
    Dynamic& operator=(bool v) { _value = v; return *this; }
    Dynamic& operator=(const char* v) { _value = String(v); return *this; }
    Dynamic& operator=(String v) { _value = std::move(v); return *this; }
    Dynamic& operator=(ObjectType v) { _value = std::move(v); return *this; }
    Dynamic& operator=(std::nullptr_t) { _value = NullType{}; return *this; }

    // ============ 类型判断 ============
    bool isNull() const { return std::holds_alternative<NullType>(_value); }
    bool isBool() const { return std::holds_alternative<bool>(_value); }
    bool isNumber() const { return std::holds_alternative<double>(_value); }
    bool isString() const { return std::holds_alternative<String>(_value); }
    bool isArray() const { return std::holds_alternative<ArrayType>(_value); }
    bool isObject() const { return std::holds_alternative<ObjectType>(_value); }
    bool isFunction() const { return std::holds_alternative<FunctionType>(_value); }

    // ============ 值获取 ============
    /**
    // 安全获取（返回 optional）
    std::optional<double> getNumber() const {
        if (auto* v = std::get_if<double>(&_value)) return *v;
        return std::nullopt;
    }

    std::optional<int> getInt() const {
        if (auto* v = std::get_if<double>(&_value)) return static_cast<int>(*v);
        return std::nullopt;
    }

    std::optional<bool> getBool() const {
        if (auto* v = std::get_if<bool>(&_value)) return *v;
        return std::nullopt;
    }

    std::optional<String> getString() const {
        if (auto* v = std::get_if<String>(&_value)) return *v;
        return std::nullopt;
    }
    **/

    // 不安全获取（若数值类型不匹配则会抛异常！）
    double asNumber() const { return std::get<double>(_value); }
    bool asBool() const { return std::get<bool>(_value); }
    const String& asString() const { return std::get<String>(_value); }
    const ArrayType& asArray() const { return std::get<ArrayType>(_value); }
    ObjectType asObject() const { return std::get<ObjectType>(_value); }
    const FunctionType& asFunction() const { return std::get<FunctionType>(_value); }

    // ============ 数组操作 ============
    Dynamic& operator[](size_t index) {
        auto& arr = std::get<ArrayType>(_value);
        if (index >= arr.size()) {
            throw std::out_of_range("数组索引越界");
        }
        return arr[index];
    }

    const Dynamic& operator[](size_t index) const {
        const auto& arr = std::get<ArrayType>(_value);
        if (index >= arr.size()) {
            throw std::out_of_range("数组索引越界");
        }
        return arr[index];
    }

    void push_back(const Dynamic& value) {
        auto& arr = ensureArray();
        arr.push_back(value);
    }

    void push_back(Dynamic&& value) {
        auto& arr = ensureArray();
        arr.push_back(std::move(value));
    }

    size_t size() const {
        if (!isArray()) return 0;
        return std::get<ArrayType>(_value).size();
    }

    bool empty() const {
        if (!isArray()) return true;
        return std::get<ArrayType>(_value).empty();
    }

    void reserve(size_t capacity) {
        ensureArray().reserve(capacity);
    }

    // 范围遍历支持
    auto begin() { return ensureArray().begin(); }
    auto end() { return ensureArray().end(); }
    auto begin() const { return std::get<ArrayType>(_value).begin(); }
    auto end() const { return std::get<ArrayType>(_value).end(); }

    // ============ 函数调用 ============
    Dynamic call(const std::vector<Dynamic>& args = {}) const {
        const auto& func = std::get<FunctionType>(_value);
        return func(args);
    }

    // ============ 类型转换 ============
    String toString() const {
        return valueToString(_value);
    }

    // 转换为布尔值（用于条件判断）
    operator bool() const {
        return std::visit(overloaded{
            [](NullType) { return false; },
            [](bool b) { return b; },
            [](double d) { return d != 0.0; },
            [](const String& s) { return !s.empty(); },
            [](const ArrayType& arr) { return !arr.empty(); },
            [](const ObjectType& obj) { return obj != nullptr; },
            [](const FunctionType&) { Exception::throwRuntimeError("不支持函数转换为布尔值"); return false; }
        }, _value);
    }

    // ============ 比较运算符 ============
    bool operator==(const Dynamic& other) const {
        // 相同类型直接比较
        if (_value.index() == other._value.index()) {
            return std::visit(overloaded{
                [](NullType, NullType) { return true; },
                [](bool a, bool b) { return a == b; },
                [](double a, double b) { return a == b; },
                [](const String& a, const String& b) { return a == b; },
                [](const ArrayType& a, const ArrayType& b) {
                    if (a.size() != b.size()) return false;
                    for (size_t i = 0; i < a.size(); ++i) {
                        if (!(a[i] == b[i])) return false;
                    }
                    return true;
                },
                [](const ObjectType& a, const ObjectType& b) { return a == b; },
                [](const FunctionType&, const FunctionType&) { return false; },
                [](auto, auto) { Exception::throwRuntimeError("不支持函数之间的比较"); return false; }  // 不会发生
            }, _value, other._value);
        }
        return false;
    }

    bool operator!=(const Dynamic& other) const { return !(*this == other); }

    bool operator<(const Dynamic& other) const {
        if (_value.index() != other._value.index()) {
            Exception::throwRuntimeError("不支持不同类型之间的比较");
            return false;
        }
        return std::visit(overloaded{
            [](double a, double b) { return a < b; },
            [](const String& a, const String& b) { return a < b; },
            [](auto, auto) -> bool { Exception::throwRuntimeError("该类型不支持比较！"); return false;}
        }, _value, other._value);
    }

    bool operator>(const Dynamic& other) const { return other < *this; }
    bool operator<=(const Dynamic& other) const { return !(*this > other); }
    bool operator>=(const Dynamic& other) const { return !(*this < other); }

    // ============ 逻辑运算符 ============
    Dynamic operator&&(const Dynamic& other) const {
        return Dynamic(asBool() && other.asBool());
    }

    Dynamic operator||(const Dynamic& other) const {
        return Dynamic(asBool() || other.asBool());
    }

    Dynamic operator!() const {
        return Dynamic(!asBool());
    }

    // ============ 算术运算符 ============
    Dynamic operator+(const Dynamic& other) const {
        return std::visit(overloaded{
            // 数字 + 数字
            [](double a, double b) -> Dynamic { return Dynamic(a + b); },
            // 字符串 + 字符串
            [](const String& a, const String& b) -> Dynamic { return Dynamic(a + b); },
            // 数字 + 字符串
            [](double a, const String& b) -> Dynamic { return Dynamic(valueToString(DynamicValue(a)) + b); },
            // 字符串 + 数字
            [](const String& a, double b) -> Dynamic { return Dynamic(a + valueToString(DynamicValue(b))); },
            // 其他组合不支持
            [](auto, auto) -> Dynamic { throw std::runtime_error("不支持的加法操作"); }
        }, _value, other._value);
    }

    Dynamic operator-(const Dynamic& other) const {
        return Dynamic(asNumber() - other.asNumber());
    }

    Dynamic operator-() const {
        return std::visit(overloaded{
            [](double a) -> Dynamic {
                return Dynamic(-a);
            },
            [](auto) -> Dynamic {
                Exception::throwRuntimeError("该数值不能用于取负数！");
            }
        }, _value);
    }

    Dynamic operator*(const Dynamic& other) const {
        return Dynamic(asNumber() * other.asNumber());
    }

    Dynamic operator/(const Dynamic& other) const {
        double divisor = other.asNumber();
        if (divisor == 0.0) throw std::runtime_error("除数不能为零");
        return Dynamic(asNumber() / divisor);
    }

    Dynamic operator%(const Dynamic& other) const {
        double divisor = other.asNumber();
        if (divisor == 0.0) throw std::runtime_error("模数不能为零");
        return Dynamic(std::fmod(asNumber(), divisor));
    }

    // ============ 复合赋值运算符 ============
    Dynamic& operator+=(const Dynamic& other) { *this = *this + other; return *this; }
    Dynamic& operator-=(const Dynamic& other) { *this = *this - other; return *this; }
    Dynamic& operator*=(const Dynamic& other) { *this = *this * other; return *this; }
    Dynamic& operator/=(const Dynamic& other) { *this = *this / other; return *this; }

    // ============ 析构函数 ============
    ~Dynamic() = default;

private:
    // 确保是数组类型，如果不是则转换
    ArrayType& ensureArray() {
        if (!isArray()) {
            if (isNull()) {
                _value = ArrayType{};
            } else {
                throw std::runtime_error("不是数组类型");
            }
        }
        return std::get<ArrayType>(_value);
    }
};