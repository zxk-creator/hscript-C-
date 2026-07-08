//
// Created by kkplay on 7/7/26.
//

#pragma once

// 所有脚本对象的基类
// HClass(类定义)和HInstance(实例)都继承自它，
// 使得Dynamic的variant可以用一个ObjectType槽位存储两者
class HObject
{
public:
    virtual ~HObject() = default;
};
