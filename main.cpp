#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include "token/Scanner.h"
#include "ast/Parser.h"
#include "ast/Interpreter.h"
#include "lib/ScriptLibs.h"

int main(int argc, char* argv[]) {

    if (argc != 2)
    {
        std::cerr << "错误：必须传入一个且只能一个脚本绝对路径！" << std::endl;
        std::cerr << "用法: " << argv[0] << " <绝对路径>" << std::endl;
        return 1;
    }

    std::string path = argv[1];
    std::filesystem::path fs_path(path);
    if (!fs_path.is_absolute()) {
        std::cerr << "错误：传入的路径必须是绝对路径" << std::endl;
        return 1;
    }

    if (!std::filesystem::exists(fs_path)) {
        std::cerr << "错误：路径不存在: " << path << std::endl;
        return 1;
    }

    std::ifstream file(path);
    if (!file.is_open())
    {
        std::cerr << "错误：无法打开文件: " << path << std::endl;
        return 1;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    std::string source = buffer.str();
    file.close();

    // 1扫描
    Scanner scanner(source);
    auto tokens = scanner.scanTokens();

    // 2解析
    Parser parser(tokens);
    auto statements = parser.parse();

    // 3注册原生函数
    Interpreter interpreter;

    interpreter.registerNativeFunc("trace", [](const std::vector<Dynamic>& args) -> Dynamic {
        if (!args.empty()) {
            std::cout << args[0].toString() << std::endl;
        } else {
            std::cout << std::endl;
        }
        return Dynamic();
    });

    interpreter.registerNativeFunc("now",[](const std::vector<Dynamic>& args) -> Dynamic
    {
       return ScriptLibs::now();
    });

    interpreter.registerNativeClass("TestClass", {});

    // 4解释执行
    interpreter.interpret(statements);

    return 0;
}
