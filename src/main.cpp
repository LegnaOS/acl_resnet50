/**
* @file main.cpp
*
* Copyright (C) 2020. Huawei Technologies Co., Ltd. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include <iostream>
#include "sample_process.h"
#include "utils.h"
using namespace std;
bool g_isDevice = false;

int main()
{
    SampleProcess processSample;  //两个用处  程序初始化  事例运行
	// 初始化程序的运行环境 
    Result ret = processSample.InitResource();
    if (ret != SUCCESS) {
        ERROR_LOG("sample init resource failed");
        return FAILED;
    }
	// 运行示例
    ret = processSample.Process();
    if (ret != SUCCESS) {
        ERROR_LOG("sample process failed");
        return FAILED;
    }

    INFO_LOG("execute sample success");
    return SUCCESS;
}

、

// 初始化：
// 