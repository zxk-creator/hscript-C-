#include <iostream>
#include <string>
#include "token/Scanner.h"
#include "ast/Parser.h"
#include "ast/Interpreter.h"

int main() {

    std::string source = R"(
        // ======== 基本类 ========
        class Animal {
            function new(n: String): Void {
                this.name = n;
            }

            function speak(): String {
                return this.name + " makes a sound";
            }

            function parentSpeak(): String {
                return "parent says: " + this.name;
            }
        }

        // ======== 继承 ========
        class Dog extends Animal {
            function new(n: String): Void {
                this.name = n;
                this.legs = 4;
            }

            function speak(): String {
                return this.name + " barks";
            }

            function getLegs(): Int {
                return this.legs;
            }

            // 测试super调用
            function callParentSpeak(): String {
                return super.parentSpeak();
            }
        }

        // ======== 多层继承 ========
        class Puppy extends Dog {
            function new(n: String): Void {
                this.name = n;
            }

            function speak(): String {
                return this.name + " yaps!";
            }
        }

        // ======== 注解和修饰符 ========
        @Override
        public final class Calculator {
            public function new(): Void {
                this.result = 0;
            }

            function add(a: Int, b: Int): Int {
                return a + b;
            }

            public function calculate(x: Int): Int {
                this.result = add(x, 5);
                return this.result;
            }
        }

        // ======== 测试 ========
        var animal = new Animal("Generic");
        trace(animal.speak());

        var dog = new Dog("Rex");
        trace(dog.speak());
        trace("legs: " + dog.getLegs());
        trace(dog.callParentSpeak());
        dog.legs = 3;
        trace("legs after set: " + dog.legs);

        var puppy = new Puppy("Max");
        trace(puppy.speak());

        var calc = new Calculator();
        trace("calc result: " + calc.calculate(10));
        trace("calc.calculate(3): " + calc.calculate(3));

        trace("所有测试完成！");
    )";

    // 1. 扫描
    Scanner scanner(source);
    auto tokens = scanner.scanTokens();

    // 2. 解析
    Parser parser(tokens);
    auto statements = parser.parse();

    // 3. 注册原生函数
    Interpreter interpreter;

    // 注册 trace 函数：打印字符串到控制台
    interpreter.registerNative("trace", [](const std::vector<Dynamic>& args) -> Dynamic {
        if (!args.empty()) {
            std::cout << args[0].toString() << std::endl;
        } else {
            std::cout << std::endl;
        }
        return Dynamic();
    });

    // 4. 解释执行
    interpreter.interpret(statements);

    return 0;
}