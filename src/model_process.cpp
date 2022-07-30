/**
* @file model_process.cpp
*
* Copyright (C) 2020. Huawei Technologies Co., Ltd. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "model_process.h"
#include <iostream>
#include <map>
#include <sstream>
#include <algorithm>
#include "utils.h"
using namespace std;
extern bool g_isDevice;
//构造函数中初始化了参数的初始值 包括了模型ID 内存大小 Model权值，模型内存指针 模型权值指针，加载标识，模型描述信息 输入信息 ，输出信息
ModelProcess::ModelProcess() :modelId_(0), modelMemSize_(0), modelWeightSize_(0), modelMemPtr_(nullptr),
modelWeightPtr_(nullptr), loadFlag_(false), modelDesc_(nullptr), input_(nullptr), output_(nullptr)
{
}

ModelProcess::~ModelProcess()
{
    Unload();
    DestroyDesc();
    DestroyInput();
    DestroyOutput();
}

// 主要是分配内存 加载模型
Result ModelProcess::LoadModelFromFileWithMem(const char *modelPath)
{
	// 防止重复加载
    if (loadFlag_) {
        ERROR_LOG("has already loaded a model");
        return FAILED;
    }
	// 根据模型文件
	// 获取模型执行时所需的权值内存大小、工作内存大小，查询
	// modelMemSize_ 工作内存 ；modelWeightSize_ 权值内存
    aclError ret = aclmdlQuerySize(modelPath, &modelMemSize_, &modelWeightSize_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("query model failed, model file is %s", modelPath);
        return FAILED;
    }
	// 查询后。在Device申请modelMemSize_大小的线性内存，modelMemPtr_ 返回已分配内存的指针

    ret = aclrtMalloc(&modelMemPtr_, modelMemSize_, ACL_MEM_MALLOC_HUGE_FIRST); //大页内存
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("malloc buffer for mem failed, require size is %zu", modelMemSize_);
        return FAILED;
    }
	// 与上述相同 分配权值内存大小的内存块给到modelWeightPtr_
    ret = aclrtMalloc(&modelWeightPtr_, modelWeightSize_, ACL_MEM_MALLOC_HUGE_FIRST);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("malloc buffer for weight failed, require size is %zu", modelWeightSize_);
        return FAILED;
    }
	// 从文件加载离线模型数据（适配昇腾AI处理器的离线模型） 
	// 输入工作内存 权值内存的指针以及大小
	// 系统完成模型加载后，返回的modelId_，作为后续操作时用于识别模型的标志
	// 将om文件的数据读取到内存中 并且返回一个标识符 modelId_
    ret = aclmdlLoadFromFileWithMem(modelPath, &modelId_, modelMemPtr_,
        modelMemSize_, modelWeightPtr_, modelWeightSize_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("load model from file failed, model file is %s", modelPath);
        return FAILED;
    }

    loadFlag_ = true;
    INFO_LOG("load model %s success", modelPath);
    return SUCCESS;
}
//模型文件加载进来以后，进行描述
// 初始化模型描述信息
Result ModelProcess::CreateDesc()
{
	// 创建接受模型描述信息的空间。
    modelDesc_ = aclmdlCreateDesc();
    if (modelDesc_ == nullptr) {
        ERROR_LOG("create model description failed");
        return FAILED;
    }
    //从描述信息中
	// 获取该模型的模型描述信息 存放在modelDesc_中 传入前面获取的modelID 就可以返回所有的模型描述信息
    aclError ret = aclmdlGetDesc(modelDesc_, modelId_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("get model description failed");
        return FAILED;
    }

    INFO_LOG("create model description success");

    return SUCCESS;
}

void ModelProcess::DestroyDesc()
{
    if (modelDesc_ != nullptr) {
        (void)aclmdlDestroyDesc(modelDesc_);
        modelDesc_ = nullptr;
    }
}

// 
Result ModelProcess::CreateInput(void *inputDataBuffer, size_t bufferSize)
{
	// 创建一个用于描述模型推理时的输入数据、输出数据的结构（aclmdlDataset类型）
    input_ = aclmdlCreateDataset();// 此input_只存放输入
    if (input_ == nullptr) {
        ERROR_LOG("can't create dataset, create input failed");
        return FAILED;
    }
	// 通过输入参数的内存 来初始化inputData（aclDataBuffer类型）
    aclDataBuffer* inputData = aclCreateDataBuffer(inputDataBuffer, bufferSize);
    if (inputData == nullptr) {
        ERROR_LOG("can't create data buffer, create input failed");
        return FAILED;
    }
	// 将数据保存到input_ （把inputData，放到input_ 中（aclmdlDataset类型））
    aclError ret = aclmdlAddDatasetBuffer(input_, inputData);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("add input dataset buffer failed");
        aclDestroyDataBuffer(inputData);
        inputData = nullptr;
        return FAILED;
    }

    return SUCCESS;
}

void ModelProcess::DestroyInput()
{
    if (input_ == nullptr) {
        return;
    }

    for (size_t i = 0; i < aclmdlGetDatasetNumBuffers(input_); ++i) {
        aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(input_, i);
        aclDestroyDataBuffer(dataBuffer);
    }
    aclmdlDestroyDataset(input_);
    input_ = nullptr;
}

Result ModelProcess::CreateOutput()
{
	// 需要有模型描述信息
    if (modelDesc_ == nullptr) {
        ERROR_LOG("no model description, create ouput failed");
        return FAILED;
    }
	// 创建一个用于描述模型推理时的输入数据、输出数据的结构（aclmdlDataset类型）
    output_ = aclmdlCreateDataset(); // 此output_只存放输出
    if (output_ == nullptr) {
        ERROR_LOG("can't create dataset, create output failed");
        return FAILED;
    }
	// 通过模型描述信息获取模型的输出个数。
    size_t outputSize = aclmdlGetNumOutputs(modelDesc_);
	
    for (size_t i = 0; i < outputSize; ++i) {
		// 根据模型描述信息获取第i个输出的字节数
        size_t buffer_size = aclmdlGetOutputSizeByIndex(modelDesc_, i);

        void *outputBuffer = nullptr;
		// 申请内存
        aclError ret = aclrtMalloc(&outputBuffer, buffer_size, ACL_MEM_MALLOC_NORMAL_ONLY);//申请了普通页内存
        if (ret != ACL_ERROR_NONE) {
            ERROR_LOG("can't malloc buffer, size is %zu, create output failed", buffer_size);
            return FAILED;
        }
		// 通过申请的内存 来初始化outputData（aclDataBuffer类型）
        aclDataBuffer* outputData = aclCreateDataBuffer(outputBuffer, buffer_size);
        if (ret != ACL_ERROR_NONE) {
            ERROR_LOG("can't create data buffer, create output failed");
            aclrtFree(outputBuffer);
            return FAILED;
        }
		// 将数据保存到output_ （把outputData，放到output_中（aclmdlDataset类型））
        ret = aclmdlAddDatasetBuffer(output_, outputData);
        if (ret != ACL_ERROR_NONE) {
            ERROR_LOG("can't add data buffer, create output failed");
            aclrtFree(outputBuffer);
            aclDestroyDataBuffer(outputData);
            return FAILED;
        }
    }

    INFO_LOG("create model output success");
    return SUCCESS;
}

void ModelProcess::DumpModelOutputResult()
{
    stringstream ss;
    size_t outputNum = aclmdlGetDatasetNumBuffers(output_);
    static int executeNum = 0;
    for (size_t i = 0; i < outputNum; ++i) {
        ss << "output" << ++executeNum << "_" << i << ".bin";
        string outputFileName = ss.str();
        FILE *outputFile = fopen(outputFileName.c_str(), "wb");
        if (outputFile) {
            aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(output_, i);
            void* data = aclGetDataBufferAddr(dataBuffer);
            uint32_t len = aclGetDataBufferSize(dataBuffer);

            void* outHostData = NULL;
            aclError ret = ACL_ERROR_NONE;
            if (!g_isDevice) {
                ret = aclrtMallocHost(&outHostData, len);
                if (ret != ACL_ERROR_NONE) {
                    ERROR_LOG("aclrtMallocHost failed, ret[%d]", ret);
                    return;
                }

                ret = aclrtMemcpy(outHostData, len, data, len, ACL_MEMCPY_DEVICE_TO_HOST);
                if (ret != ACL_ERROR_NONE) {
                    ERROR_LOG("aclrtMemcpy failed, ret[%d]", ret);
                    (void)aclrtFreeHost(outHostData);
                    return;
                }

                fwrite(outHostData, len, sizeof(char), outputFile);

                ret = aclrtFreeHost(outHostData);
                if (ret != ACL_ERROR_NONE) {
                    ERROR_LOG("aclrtFreeHost failed, ret[%d]", ret);
                    return;
                }
            } else {
                fwrite(data, len, sizeof(char), outputFile);
            }
            fclose(outputFile);
            outputFile = nullptr;
        } else {
            ERROR_LOG("create output file [%s] failed", outputFileName.c_str());
            return;
        }
    }

    INFO_LOG("dump data success");
    return;
}

void ModelProcess::OutputModelResult()
{
	// 遍历Output 获取结果 类似与数组遍历 
    for (size_t i = 0; i < aclmdlGetDatasetNumBuffers(output_); ++i) {
		// 结果保存在dataBuffer中
        aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(output_, i);
        void* data = aclGetDataBufferAddr(dataBuffer);
        uint32_t len = aclGetDataBufferSize(dataBuffer);

        void *outHostData = NULL;
        aclError ret = ACL_ERROR_NONE;
        float *outData = NULL;
        if (!g_isDevice) {
            aclError ret = aclrtMallocHost(&outHostData, len);
            if (ret != ACL_ERROR_NONE) {
                ERROR_LOG("aclrtMallocHost failed, ret[%d]", ret);
                return;
            }
			// 将结果拷贝出来
            ret = aclrtMemcpy(outHostData, len, data, len, ACL_MEMCPY_DEVICE_TO_HOST);
            if (ret != ACL_ERROR_NONE) {
                ERROR_LOG("aclrtMemcpy failed, ret[%d]", ret);
                return;
            }

            outData = reinterpret_cast<float*>(outHostData);
        } else {
            outData = reinterpret_cast<float*>(data);
        }
		// 拿着所有的结果保存在resultMap中 
        map<float, unsigned int, greater<float> > resultMap;
        for (unsigned int j = 0; j < len / sizeof(float); ++j) {
            resultMap[*outData] = j;
            outData++;
        }
		// 输出前五个最大的值
        int cnt = 0;
        for (auto it = resultMap.begin(); it != resultMap.end(); ++it) {
            // print top 5
            if (++cnt > 5) {
                break;
            }

            INFO_LOG("top %d: index[%d] value[%lf]", cnt, it->second, it->first);
        }
        if (!g_isDevice) {
            ret = aclrtFreeHost(outHostData);
            if (ret != ACL_ERROR_NONE) {
                ERROR_LOG("aclrtFreeHost failed, ret[%d]", ret);
                return;
            }
        }
    }

    INFO_LOG("output data success");
    return;
}

void ModelProcess::DestroyOutput()
{
    if (output_ == nullptr) {
        return;
    }
    // 推理用模型的内存资源进行释放
    for (size_t i = 0; i < aclmdlGetDatasetNumBuffers(output_); ++i) {
        aclDataBuffer* dataBuffer = aclmdlGetDatasetBuffer(output_, i);
        void* data = aclGetDataBufferAddr(dataBuffer);
        (void)aclrtFree(data);
        (void)aclDestroyDataBuffer(dataBuffer);
    }

    (void)aclmdlDestroyDataset(output_);
    output_ = nullptr;
}

Result ModelProcess::Execute()
{
	// 执行模型推理，直到返回推理结果
    aclError ret = aclmdlExecute(modelId_, input_, output_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("execute model failed, modelId is %u", modelId_);
        return FAILED;
    }

    INFO_LOG("model execute success");
    return SUCCESS;
}

void ModelProcess::Unload()
{
    if (!loadFlag_) {
        WARN_LOG("no model had been loaded, unload failed");
        return;
    }

    aclError ret = aclmdlUnload(modelId_);
    if (ret != ACL_ERROR_NONE) {
        ERROR_LOG("unload model failed, modelId is %u", modelId_);
    }
    // 释放模型描述资源
    if (modelDesc_ != nullptr) {
        (void)aclmdlDestroyDesc(modelDesc_);
        modelDesc_ = nullptr;
    }
    //释放内存资源
    if (modelMemPtr_ != nullptr) {
        aclrtFree(modelMemPtr_);
        modelMemPtr_ = nullptr;
        modelMemSize_ = 0;
    }
    //释放权值内存资源
    if (modelWeightPtr_ != nullptr) {
        aclrtFree(modelWeightPtr_);
        modelWeightPtr_ = nullptr;
        modelWeightSize_ = 0;
    }

    loadFlag_ = false;
    INFO_LOG("unload model success, modelId is %u", modelId_);
}
