cmake_minimum_required(VERSION 3.20)
project(HaxeParser)

set(CMAKE_CXX_STANDARD 20)

if(MINGW)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
endif()

add_executable(HaxeParser main.cpp
        token/Token.h
        token/Scanner.h
        ast/Expr.h
        type/Dynamic.h
        type/HClass.h
        util/ExceptionUtil.h
        ast/Parser.h
        ast/Interpreter.h
        env/Environment.h
        util/reflect/Reflect.h
        test/TestClass.h
        lib/ScriptLibs.h
        lib/ScriptLibs.cpp)
