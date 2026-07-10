//
// Created by kkplay on 7/9/26.
//

#pragma once
#include <memory>
#include <string>
#include "../../test/TestClass.h"

// 反射类，用于运行时动态创建C++类（由于没有反射，我们需要手动到这里注册）
class Reflect
{
public:
    /**
     * 通过“反射”创建一个新的C++类
     * @param name C++类名
     * @param args 构造函数的参数
     * @return 新创建的C++类（基类）
     */
    static std::shared_ptr<HClass> createInstance(const std::string& name, std::vector<Dynamic>& args)
    {
        // 新增类的话，直接来这里注册一个new。
        if (name == "TestClass") return std::make_shared<TestClass>();

        return nullptr;
    }
};
