/**
* @file sample_process.cpp
*
* Copyright (C) 2020. Huawei Technologies Co., Ltd. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "sample_process.h"
#include <iostream>
#include "model_process.h"
#include "acl/acl.h"
#include "utils.h"
using namespace std;
extern bool g_isDevice;

SampleProcess::SampleProcess() :deviceId_(0), context_(nullptr), stream_(nullptr)
{
}

SampleProcess::~SampleProcess()
{
    DestroyResource();
}

// 初始化硬件环境
Result SampleProcess::InitResource()
{
    // ACL init
    const char *aclConfigPath = "../src/acl.json";
	// 初始化函数 进程环境初始化 只能调用一次
    aclError ret = aclInit(aclConfigPath);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("acl init failed");
        return FAILED;
    }
    INFO_LOG("acl init success");

    // open device
	// 指定用于运算的Device，（同时隐式创建默认Context。）
    ret = aclrtSetDevice(deviceId_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("acl open device %d failed", deviceId_);
        return FAILED;
    }
    INFO_LOG("open device %d success", deviceId_);

    // create context (set current)
	// 在当前进程或线程中显式创建一个Context.使用显式的Context增加可读性
    ret = aclrtCreateContext(&context_, deviceId_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("acl create context failed");
        return FAILED;
    }
    INFO_LOG("create context success");

    // create stream
	// 创建一个Stream
    ret = aclrtCreateStream(&stream_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("acl create stream failed");
        return FAILED;
    }
    INFO_LOG("create stream success");

    // get run mode
    aclrtRunMode runMode;
	// 获取当前昇腾AI软件栈的运行模式
    ret = aclrtGetRunMode(&runMode);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("acl get run mode failed");
        return FAILED;
    }
	// 从 运行模式是否为 ACL_DEVICE
    // 如果查询结果为ACL_HOST，则数据传输时涉及申请Host上的内存。
    // 如果查询结果为ACL_DEVICE，则数据传输时仅需申请Device上的内存。
    g_isDevice = (runMode == ACL_DEVICE);
    INFO_LOG("get run mode success");
	// 全部资源初始化完成
    return SUCCESS;
}

Result SampleProcess::Process()
{
    // model init
    ModelProcess processModel;
    const char* omModelPath = "../model/resnet50.om";
	// 1.将模型文件加载进内存  得到模型ID 为识别模型的标志
    Result ret = processModel.LoadModelFromFileWithMem(omModelPath);//返回了一个modelID
    if (ret != SUCCESS) {
        ERROR_LOG("execute LoadModelFromFileWithMem failed");
        return FAILED;
    }
	// 2.获得模型的描述信息
    ret = processModel.CreateDesc();
    if (ret != SUCCESS) {
        ERROR_LOG("execute CreateDesc failed");
        return FAILED;
    }
	// 3.从描述信息中获得模块的输出信息
    ret = processModel.CreateOutput();
    if (ret != SUCCESS) {
        ERROR_LOG("execute CreateOutput failed");
        return FAILED;
    }
	// 上面三步  得到了我们想要什么输出数据
	
    // loop begin
    string testFile[] = {
        "../data/dog1_1024_683.bin",
        "../data/dog2_1024_683.bin"
    };

    for (size_t index = 0; index < sizeof(testFile) / sizeof(testFile[0]); ++index) {
        INFO_LOG("start to process file:%s", testFile[index].c_str());
        // model process
		// 1.将文件读取至内存 得到大小和内容
        uint32_t devBufferSize;
        void *picDevBuffer = Utils::GetDeviceBufferOfFile(testFile[index], devBufferSize);
        if (picDevBuffer == nullptr) {
            ERROR_LOG("get pic device buffer failed,index is %zu", index);
            return FAILED;
        }
        // 2.将文件的内容放到输入中
        ret = processModel.CreateInput(picDevBuffer, devBufferSize);
        if (ret != SUCCESS) {
            ERROR_LOG("execute CreateInput failed");
            aclrtFree(picDevBuffer);
            return FAILED;
        }
		// 3.执行模型推理，直到返回推理结果
        ret = processModel.Execute();
        if (ret != SUCCESS) {
            ERROR_LOG("execute inference failed");
            return FAILED;
        }
	
        // print the top 5 confidence values with indexes.use function DumpModelOutputResult
        // if want to dump output result to file in the current directory
        processModel.OutputModelResult(); //打印结果
		// 释放
        aclrtFree(picDevBuffer); // 释放保存文件的内存
        processModel.DestroyInput(); // 必须释放输入 因为内存已经释放 输入无效
    }// release model input buffer
    // loop end

    return SUCCESS;
}

void SampleProcess::DestroyResource()
{
    aclError ret;
    if (stream_ != nullptr) {
        ret = aclrtDestroyStream(stream_);
        if (ret != ACL_ERROR_NONE) {
            ERROR_LOG("destroy stream failed");
        }
        stream_ = nullptr;
    }
    INFO_LOG("end to destroy stream");

    if (context_ != nullptr) {
        ret = aclrtDestroyContext(context_);
        if (ret != ACL_ERROR_NONE) {
            ERROR_LOG("destroy context failed");
        }
        context_ = nullptr;
    }
    INFO_LOG("end to destroy context");

    ret = aclrtResetDevice(deviceId_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("reset device failed");
    }
    INFO_LOG("end to reset device is %d", deviceId_);

    ret = aclFinalize();
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("finalize acl failed");
    }
    INFO_LOG("end to finalize acl");

}
