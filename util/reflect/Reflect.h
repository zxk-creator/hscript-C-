//
// Created by kkplay on 7/9/26.
//

#pragma once
#include <memory>
#include <string>
#include "../../test/TestClass.h"

// 反射类，用于运行时动态创建C++类（需要手动到这里注册）
class Reflect
{
public:
    static std::shared_ptr<HClass> createInstance(const std::string& name, std::vector<Dynamic>& args)
    {
        // 新增类的话，直接来这里注册一个new。
        if (name == "TestClass") return std::make_shared<TestClass>();

        return nullptr;
    }
};
