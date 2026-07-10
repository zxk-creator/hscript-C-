//
// Created by kkplay on 7/10/26.
//
#include "ScriptLibs.h"
#include <iostream>
#include <chrono>
#include <cmath>


// 获得自 1970‑01‑01 00:00:00 UTC 以来的秒数（保留两位小数）
double ScriptLibs::now() {
    using namespace std::chrono;
    // 获取自 epoch 以来的毫秒数
    auto ms = duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
    // 转为秒，四舍五入保留两位小数
    return std::round(ms / 10.0) / 100.0;
}

