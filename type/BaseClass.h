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

/**
*  使用示例
class Player : public HObject {
public:
    int hp = 100;

    void takeDamage(int damage) {
    hp -= damage;
    std::cout << "受到 " << damage << " 点伤害，剩余 HP: " << hp << std::endl;
    }

    int getHp() const {
    return hp;
    }

    Player() {
    // 注册 takeDamage：捕获 this，包装成统一格式
    registerMethod("takeDamage", [this](std::vector<std::any> args) -> std::any {
    int dmg = std::any_cast<int>(args[0]);
    this->takeDamage(dmg);
    return std::any{};
    });

    registerMethod("getHp", [this](std::vector<std::any> args) -> std::any {
    return std::any{this->getHp()};
    });
    }
};
*/