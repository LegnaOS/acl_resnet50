/**
* @file model_process.h
*
* Copyright (C) 2020. Huawei Technologies Co., Ltd. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#pragma once
#include <iostream>
#include "utils.h"
#include "acl/acl.h"

/**
* ModelProcess
*/
class ModelProcess {
public:
    /**
    * @brief Constructor
    */
    ModelProcess();

    /**
    * @brief Destructor
    */
    ~ModelProcess();

    /**
    * @brief load model from file with mem
    * @param [in] modelPath: model path
    * @return result
    */
    Result LoadModelFromFileWithMem(const char *modelPath);

    /**
    * @brief unload model
    */
    void Unload();

    /**
    * @brief create model desc
    * @return result
    */
    Result CreateDesc();

    /**
    * @brief destroy desc
    */
    void DestroyDesc();

    /**
    * @brief create model input
    * @param [in] inputDataBuffer: input buffer
    * @param [in] bufferSize: input buffer size
    * @return result
    */
    Result CreateInput(void *inputDataBuffer, size_t bufferSize);

    /**
    * @brief destroy input resource
    */
    void DestroyInput();

    /**
    * @brief create output buffer
    * @return result
    */
    Result CreateOutput();

    /**
    * @brief destroy output resource
    */
    void DestroyOutput();

    /**
    * @brief model execute
    * @return result
    */
    Result Execute();

    /**
    * @brief dump model output result to file
    */
    void DumpModelOutputResult();

    /**
    * @brief get model output result
    */
    void OutputModelResult();

private:
	// 模型标识符
    uint32_t modelId_;
	// 分别代表工作内存 权值内存的大小以及指向内存的指针
    size_t modelMemSize_;
    size_t modelWeightSize_;
    void *modelMemPtr_;
    void *modelWeightPtr_;
	
    bool loadFlag_;  // model load flag
	// 模型描述信息
    aclmdlDesc *modelDesc_;
	
	// 通过模型描述信息初始化的输入信息
    aclmdlDataset *input_;// input_ : 存放输入信息（里面可以看成一个链表 每一个节点都是输出信息）
	// 通过模型描述信息初始化的输出信息
    aclmdlDataset *output_;// output_ : 存放输出信息（里面可以看成一个链表 每一个节点都是输出信息）
};

