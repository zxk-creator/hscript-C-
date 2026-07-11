<div align="center">

# hscript-c

**Haxe-style scripting language interpreter embedded in C++**

[![License](https://img.shields.io/badge/license-Apache%202.0-blue.svg)](LICENSE)
[![C++20](https://img.shields.io/badge/C%2B%2B-20-blue)](https://en.cppreference.com/w/cpp/20)
[![CMake](https://img.shields.io/badge/build-CMake-blue)](CMakeLists.txt)

允许使用 Haxe 风格语法编写脚本，在 C++ 中解释执行，**且脚本类可继承 C++ 原生类、调用并重写其方法！**

*[English](#english) | [中文](#chinese)*

</div>

---

<a name="chinese"></a>

# hscript-c — 中文文档

## 📖 简介

**hscript-c** 是一个用 C++ 编写的脚本语言解释器，语法风格类似 [Haxe](https://haxe.org/) 语言。它让你可以用脚本文件定义类、方法、变量，并直接在 C++ 程序中解释执行。

### 核心特色

- **🧬 继承 C++ 原生类** — 脚本类可以直接 `extends` 一个 C++ 类，调用其方法并重写
- **🌏 中英文双语关键字** — 所有关键字同时支持英文和中文（如 `class`/`类`、`function`/`函数`）
- **⚡ 轻量嵌入** — 通过 CMake 集成，在 C++ 代码中注册原生函数和类即可使用
- **🧩 动态类型系统** — 统一的 `Dynamic` 类型，支持 null、布尔、数字、字符串、数组、对象、函数
- **🔧 完整的语言特性** — 变量、函数、if/else、while、for 循环、类、继承、数组

## 🚀 快速开始

### 构建

要求：CMake 3.20+、C++20 编译器。

## 📝 脚本语法

### 变量

```haxe
var a = 1;
var b = 2.9;
var c = "Hello, World";
var d = true;
```

### 函数

```haxe
function add(a, b) {
    return a + b;
}

trace(add(3, 5)); // 输出: 8
```

### 条件与循环

```haxe
// if/else
if (a > 10) {
    trace("大于10");
} else {
    trace("小于等于10");
}

// while 循环
var i = 0;
while (i < 5) {
    trace(i);
    i = i + 1;
}

// for 循环
for (var j = 0; j < 10; j = j + 1) {
    trace(j);
}
```

### 类定义

```haxe
class MyClass
{
    public var x:Int = 0;
    public var name:String = "default";

    public function new(x:Int, name:String)
    {
        this.x = x;
        this.name = name;
    }

    public function printInfo()
    {
        trace(name + ": " + x.toString());
    }
}

var obj = new MyClass(42, "答案");
obj.printInfo();
```

> 注：类型注解如 `:Int`、`:String`、`:Float` 目前仅作语法支持，不进行运行时类型检查。

### 继承 C++ 原生类

这是 hscript-c 最核心的特性。你需要：

1. **在 C++ 端定义一个继承 `HClass` 的类**，注册方法、字段的 getter/setter
2. **在 `Reflect::createInstance` 中注册**该类
3. **在 `main()` 中通过 `registerNativeClass` 暴露**给脚本环境
4. **在脚本中 `extends` 该类**并重写方法

#### C++ 端定义（test/TestClass.h）

```cpp
class TestClass : public HClass
{
    int a = 1;
    float b = 2.0f;

public:
    TestClass() : HClass("TestClass", nullptr)
    {
        registerNativeMethod("c", FunctionType([this](const std::vector<Dynamic>& args) -> Dynamic
        {
            if (args.size() >= 1)
                return this->c(args[0].asNumber());
            return {};
        }));
    }

    int c(int val) { return a + val; }

    Dynamic getField(std::string fieldName) override { /* ... */ }
    void setField(std::string fieldName, Dynamic value) override { /* ... */ }
    bool hasField(std::string fieldName) override { /* ... */ }
};
```

#### 注册到反射（util/reflect/Reflect.h）

```cpp
static std::shared_ptr<HClass> createInstance(const std::string& name, std::vector<Dynamic>& args)
{
    if (name == "TestClass") return std::make_shared<TestClass>();
    return nullptr;
}
```

#### 脚本端继承（test/TestExtendCppClass.hx）

```haxe
class ScriptClass extends TestClass
{
    public override function c(val)
    {
        return super.c(val) + 5;
    }
}

var cls = new ScriptClass();
trace(cls.c(10)); // 输出: 16  (1 + 10 + 5)
```

### 中英文双语关键字对照

在代码中，你可以混用英文和中文关键字。当前支持的全部关键字：

| 英文 | 中文 | 用途 |
|------|------|------|
| `class` | `类` | 定义类 |
| `extends` | `继承` | 继承父类 |
| `function` | `函数` | 定义函数/方法 |
| `var` | `变量` | 声明变量 |
| `if` | `如果` | 条件分支 |
| `else` | `否则` | 条件分支 |
| `while` | `当` | 循环 |
| `for` | `循环` | 循环 |
| `return` | `返回` | 返回值 |
| `true` | `真` | 布尔真值 |
| `false` | `假` | 布尔假值 |
| `null` | `空` | 空值 |
| `this` | `自己` | 当前实例引用 |
| `super` | `父类` | 父类引用 |
| `new` | `新的` | 创建实例 |
| `&&` | `和` | 逻辑与 |
| `\|\|` | `或` | 逻辑或 |
| `public` | `公开` | 访问修饰符 |
| `private` | `私有` | 访问修饰符 |
| `protected` | `受保护` | 访问修饰符 |
| `override` | `重写` | 方法重写 |
| `final` | `最终` | final 修饰 |
| `inline` | `内联` | inline 修饰 |
| `macro` | `宏` | macro 修饰 |
| `static` | `静态` | 静态修饰 |

**中文脚本示例**（完整版见 `test/TestScriptChinese.hx`）：

```haxe
类 测试类
{
    公开 变量 甲:Int = 10;
    公开 变量 乙:Float = 20.5;
    公开 变量 丙:String = "你好，世界";

    公开 函数 新的(甲:Int, 乙:Float, 丙:String)
    {
        自己.甲 = 甲;
        自己.乙 = 乙;
        自己.丙 = 丙;
    }

    公开 函数 打印日志()
    {
        trace(甲.转字符串() + ", " + 乙.转字符串() + ", " + "!");
        trace(自己.丙);
    }
}

变量 实例 = 新的 测试类(15, 30.75, "测试完成");
实例.打印日志();
```

### 内置函数

| 函数 | 说明 |
|------|------|
| `trace(value)` | 输出值到控制台（类似 `console.log`） |
| `now()` | 返回当前时间戳（自 1970 年以来的秒数，保留两位小数） |

### 基本类型方法

所有基本类型（数字、字符串等）均支持 `.toString()`（中文：`.转字符串()`）方法。

## 🏗 项目结构

```
hscript-c/
├── token/                   # 词法分析
│   ├── Token.h              # Token 类型定义
│   └── Scanner.h            # 扫描器（将源码 → Token 流）
├── ast/                     # 抽象语法树
│   ├── Expr.h               # AST 节点定义（表达式 + 语句）
│   ├── Parser.h             # 解析器（Token 流 → AST）
│   └── Interpreter.h        # 解释器（AST → 执行 + 原生函数注册）
├── env/
│   └── Environment.h        # 变量作用域管理
├── type/
│   ├── Dynamic.h            # 动态类型系统
│   ├── HClass.h             # 脚本类定义与实例
│   └── BaseClass.h          # HObject 基类
├── util/
│   ├── ExceptionUtil.h      # 异常工具
│   └── reflect/
│       └── Reflect.h        # C++ 类反射注册
├── lib/
│   ├── ScriptLibs.h         # 脚本库函数声明
│   └── ScriptLibs.cpp       # 脚本库函数实现
├── test/                    # 测试脚本
│   ├── TestScript.hx        # 基本类测试
│   ├── TestScriptChinese.hx # 中文关键字测试
│   ├── TestExtendCppClass.hx# 继承 C++ 类测试
│   └── TestClass.h          # 供脚本继承的 C++ 测试类
├── main.cpp                 # 入口 + 原生函数注册
├── CMakeLists.txt           # CMake 构建配置
├── LICENSE                  # Apache 2.0 许可证
└── README.md                # 本文件
```

## 🧠 工作原理

hscript-c 是经典的「扫描 → 解析 → 解释」三步式解释器：

```
源码文本 → Scanner → Token 流 → Parser → AST → Interpreter → 执行结果
                                        ↑               ↑
                                  语法错误处理       原生函数/类注册
```

1. **Scanner（扫描器）** — 将脚本源码拆分为 Token 序列，识别关键字（同时支持中英文）、标识符、数字、字符串、运算符等
2. **Parser（解析器）** — 将 Token 序列按照 Haxe 语法规则构建为抽象语法树（AST），遇到语法错误时会尝试恢复并继续解析
3. **Interpreter（解释器）** — 遍历 AST 节点并执行，管理变量作用域链、函数调用栈、类实例化与继承关系。通过访问者模式实现

### 继承 C++ 类的实现机制

脚本类继承 C++ 原生类时，解释器会：
1. 解析脚本时识别 `extends NativeClass`
2. 在 `visitClassStmt` 中查找 C++ 注册的类信息
3. 通过 `Reflect::createInstance` 创建 C++ 端实例
4. 创建 `HInstance` 包裹 C++ 对象，通过 `getField`/`setField`/`findMethod` 实现跨语言属性访问和方法调用
5. 脚本方法可以调用 `super.method()` 调用 C++ 父类方法

## 🔌 扩展：注册更多 C++ 原生类

1. 创建类继承 `HClass`，实现 `getField`/`setField`/`hasField`，用 `registerNativeMethod` 注册方法
2. 在 `Reflect::createInstance` 中添加分支
3. 在 `main()` 中调用 `interpreter.registerNativeClass("ClassName", {})`
4. 编写脚本 `extends ClassName` 并使用

---

<a name="english"></a>

# hscript-c — English Documentation

## 📖 Overview

**hscript-c** is a Haxe-style scripting language interpreter written in C++. It allows you to write scripts using Haxe-like syntax and execute them within a C++ host program.

### Key Features

- **🧬 Native C++ Class Inheritance** — Script classes can `extends` C++ classes, calling and overriding their methods
- **🌏 Bilingual Keywords** — All keywords support both English and Chinese (e.g., `class`/`类`, `function`/`函数`)
- **⚡ Lightweight Embedding** — Integrate via CMake, register native functions and classes in C++ code
- **🧩 Dynamic Type System** — Unified `Dynamic` type: null, bool, number, string, array, object, function
- **🔧 Full Language Features** — Variables, functions, if/else, while/for loops, classes, inheritance, arrays

## 🚀 Quick Start

### Build

Requires: CMake 3.20+, C++20 compiler (MSVC, GCC, Clang).

## 📝 Script Syntax

### Variables

```haxe
var a = 1;
var b = 2.9;
var c = "Hello, World";
var d = true;
```

### Functions

```haxe
function add(a, b) {
    return a + b;
}

trace(add(3, 5)); // prints: 8
```

### Conditionals & Loops

```haxe
// if/else
if (a > 10) {
    trace("greater than 10");
} else {
    trace("less or equal to 10");
}

// while loop
var i = 0;
while (i < 5) {
    trace(i);
    i = i + 1;
}

// for loop
for (var j = 0; j < 10; j = j + 1) {
    trace(j);
}
```

### Classes

```haxe
class MyClass
{
    public var x:Int = 0;
    public var name:String = "default";

    public function new(x:Int, name:String)
    {
        this.x = x;
        this.name = name;
    }

    public function printInfo()
    {
        trace(name + ": " + x.toString());
    }
}

var obj = new MyClass(42, "Answer");
obj.printInfo();
```

> Note: Type annotations (`:Int`, `:String`, etc.) are syntactically supported but not enforced at runtime.

### Inheriting Native C++ Classes

This is the core feature of hscript-c:

1. **Define a C++ class extending `HClass`**, register methods and field accessors
2. **Register it in `Reflect::createInstance`**
3. **Expose it to scripts** via `registerNativeClass` in `main()`
4. **In your script**, `extends` the native class and override methods

#### C++ Side (test/TestClass.h)

```cpp
class TestClass : public HClass
{
    int a = 1;
    float b = 2.0f;

public:
    TestClass() : HClass("TestClass", nullptr)
    {
        registerNativeMethod("c", FunctionType([this](const std::vector<Dynamic>& args) -> Dynamic
        {
            if (args.size() >= 1)
                return this->c(args[0].asNumber());
            return {};
        }));
    }

    int c(int val) { return a + val; }
    Dynamic getField(std::string fieldName) override { /* ... */ }
    void setField(std::string fieldName, Dynamic value) override { /* ... */ }
    bool hasField(std::string fieldName) override { /* ... */ }
};
```

#### Reflect Registration (util/reflect/Reflect.h)

```cpp
static std::shared_ptr<HClass> createInstance(const std::string& name, std::vector<Dynamic>& args)
{
    if (name == "TestClass") return std::make_shared<TestClass>();
    return nullptr;
}
```

#### Script Side (test/TestExtendCppClass.hx)

```haxe
class ScriptClass extends TestClass
{
    public override function c(val)
    {
        return super.c(val) + 5;
    }
}

var cls = new ScriptClass();
trace(cls.c(10)); // prints: 16  (1 + 10 + 5)
```

### Chinese Keyword Equivalents

| English | Chinese | Purpose |
|---------|---------|---------|
| `class` | `类` | Define a class |
| `extends` | `继承` | Extend a parent class |
| `function` | `函数` | Define a function / method |
| `var` | `变量` | Declare a variable |
| `if` | `如果` | Conditional branch |
| `else` | `否则` | Conditional branch |
| `while` | `当` | Loop |
| `for` | `循环` | Loop |
| `return` | `返回` | Return a value |
| `true` | `真` | Boolean true |
| `false` | `假` | Boolean false |
| `null` | `空` | Null value |
| `this` | `自己` | Current instance reference |
| `super` | `父类` | Parent class reference |
| `new` | `新的` | Create an instance |
| `&&` | `和` | Logical AND |
| `\|\|` | `或` | Logical OR |
| `public` | `公开` | Access modifier |
| `private` | `私有` | Access modifier |
| `protected` | `受保护` | Access modifier |
| `override` | `重写` | Method override |
| `final` | `最终` | Final modifier |
| `static` | `静态` | Static modifier |

### Built-in Functions

| Function | Description |
|----------|-------------|
| `trace(value)` | Print value to console |
| `now()` | Return current timestamp (seconds since epoch, 2 decimal places) |

## 🏗 Project Structure

```
hscript-c/
├── token/                   # Lexical analysis
│   ├── Token.h              # Token type definitions
│   └── Scanner.h            # Scanner (source → tokens)
├── ast/                     # Abstract Syntax Tree
│   ├── Expr.h               # AST node definitions
│   ├── Parser.h             # Parser (tokens → AST)
│   └── Interpreter.h        # Interpreter (AST → execution)
├── env/
│   └── Environment.h        # Variable scope management
├── type/
│   ├── Dynamic.h            # Dynamic type system
│   ├── HClass.h             # Script class definition & instances
│   └── BaseClass.h          # HObject base class
├── util/
│   ├── ExceptionUtil.h      # Exception helpers
│   └── reflect/
│       └── Reflect.h        # C++ native class reflection
├── lib/
│   ├── ScriptLibs.h         # Script library declarations
│   └── ScriptLibs.cpp       # Script library implementations
├── test/                    # Test scripts
│   ├── TestScript.hx        # Basic class test
│   ├── TestScriptChinese.hx # Chinese keyword test
│   ├── TestExtendCppClass.hx# C++ class inheritance test
│   └── TestClass.h          # C++ test class for inheritance
├── main.cpp                 # Entry point + native registration
├── CMakeLists.txt           # CMake build configuration
├── LICENSE                  # Apache 2.0 License
└── README.md                # This file
```

## 🧠 How It Works

hscript-c follows the classic three-stage interpreter pattern:

```
Source Code → Scanner → Tokens → Parser → AST → Interpreter → Result
                                        ↑               ↑
                                  Error Recovery   Native Bindings
```

1. **Scanner** — Tokenizes source code, recognizes bilingual keywords, identifiers, literals, and operators
2. **Parser** — Builds an AST from tokens following Haxe grammar rules with error recovery
3. **Interpreter** — Walks the AST using the Visitor pattern, manages scope chains, call stacks, class instantiation, and native C++ interop

### Native C++ Class Inheritance

When a script class `extends` a native C++ class:
1. The parser identifies the superclass name
2. `visitClassStmt` looks up the native class registration
3. `Reflect::createInstance` creates the C++ native object
4. An `HInstance` wraps the C++ object, using `getField`/`setField`/`findMethod` for cross-language access
5. Script methods can call `super.method()` to invoke the C++ parent's implementation

## 🔌 Extending: Adding More Native C++ Classes

1. Create a class extending `HClass` with `getField`/`setField`/`hasField` overrides, register methods via `registerNativeMethod`
2. Add a branch in `Reflect::createInstance`
3. Call `interpreter.registerNativeClass("ClassName", {})` in `main()`
4. Write a script `extends ClassName` and use it

---

<div align="center">
Built with ❤️ using C++20
</div>
