/**
* @file sample_process.h
*
* Copyright (C) 2020. Huawei Technologies Co., Ltd. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#pragma once
#include "utils.h"
#include "acl/acl.h"

/**
* SampleProcess
*/
class SampleProcess {
public:
    /**
    * @brief Constructor //构造函数
    */
    SampleProcess();

    /**
    * @brief Destructor  //析构函数
    */
    ~SampleProcess();

    /**
    * @brief init reousce  //资源初始化 硬件/软件环境
    * @return result
    */
    Result InitResource();

    /**
    * @brief sample process //程序主要执行过程
    * @return result
    */
    Result Process();

private:
    void DestroyResource();  //资源销毁

    int32_t deviceId_;		// 初始化 此示例未做其他调用
    aclrtContext context_; 	// 初始化 此示例未做其他调用
    aclrtStream stream_;	// 初始化 此示例未做其他调用
};

