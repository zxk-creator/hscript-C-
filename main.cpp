#include <iostream>
#include <string>
#include "token/Scanner.h"
#include "ast/Parser.h"
#include "ast/Interpreter.h"

int main() {

    std::string source = R"(
        var a = 1;
        var b = 2;
        {
            var a = "inner";
        }
        a = 100;
    )";

    // 1. 扫描
    Scanner scanner(source);
    auto tokens = scanner.scanTokens();

    // 2. 解析
    Parser parser(tokens);
    auto statements = parser.parse();

    // 3. 解释执行
    Interpreter interpreter;
    interpreter.interpret(statements);

    return 0;
}