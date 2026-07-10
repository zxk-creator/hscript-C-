//
// Created by kkplay on 7/9/26.
//

#pragma once
#include "../type/Dynamic.h"
#include "../type/HClass.h"

class TestClass : public HClass
{
    int a = 1;
    float b = 2.0f;

public:
    TestClass() : HClass("TestClass",nullptr)
    {
        registerNativeMethod("c", FunctionType([this](const std::vector<Dynamic>& args) -> Dynamic
        {
            if (args.size() >= 1)
                return this->c(args[0].asNumber());
            else
                return {};
        }));
    }

    int c(int val)
    {

        return a + val;
    }

    Dynamic getField(std::string fieldName) override
    {
        if (fieldName == "a") return a;
        if (fieldName == "b") return b;

        return HClass::getField(fieldName);
    }

    void setField(std::string fieldName, Dynamic value) override
    {
        if (fieldName == "a") { a = value.asNumber(); return; }
        if (fieldName == "b") { b = value.asNumber(); return; }

        HClass::setField(fieldName, value);
    }

    bool hasField(std::string fieldName) override
    {
        if (fieldName == "a" || fieldName == "b") return true;

        return false;
    }
};
