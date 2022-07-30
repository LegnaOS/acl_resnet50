/**
* @file utils.cpp
*
* Copyright (C) 2020. Huawei Technologies Co., Ltd. All rights reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/
#include "utils.h"
#include <iostream>
#include <fstream>
#include <cstring>
#include "acl/acl.h"
#include <sys/stat.h>

extern bool g_isDevice;

// 将文件的内容读取至内存 并且返回指针和大小
void* Utils::ReadBinFile(std::string fileName, uint32_t &fileSize)
{
    struct stat sBuf;
	// 获取文件信息 保存到sBuf
    int fileStatus = stat(fileName.data(), &sBuf);
    if (fileStatus == -1) { // 获取信息正常
        ERROR_LOG("failed to get file");
        return nullptr;
    }
	// 如果不是一般文件
    if (S_ISREG(sBuf.st_mode) == 0) {
        ERROR_LOG("%s is not a file, please enter a file", fileName.c_str());
        return nullptr;
    }
	// 打开文件
    std::ifstream binFile(fileName, std::ifstream::binary);
    if (binFile.is_open() == false) {
        ERROR_LOG("open file %s failed", fileName.c_str());
        return nullptr;
    }
	// 拿到文件大小
    binFile.seekg(0, binFile.end);
    uint32_t binFileBufferLen = binFile.tellg();
    if (binFileBufferLen == 0) {
        ERROR_LOG("binfile is empty, filename is %s", fileName.c_str());
        binFile.close();
        return nullptr;
    }
	// 重新偏移到文件开头
    binFile.seekg(0, binFile.beg);

    void* binFileBufferData = nullptr;
    aclError ret = ACL_ERROR_NONE;
	// 如果设备的 runMode ！= ACL_DEVICE
    if (!g_isDevice) { 
		// 申请内存 申请大小binFileBufferLen给binFileBufferData
		// 在Host上运行时，调用该接口申请的是Host内存，由系统保证内存首地址64字节对齐。
		// 在Device上运行时，调用该接口申请的是Device内存 且Device上的内存按普通页申请
        ret = aclrtMallocHost(&binFileBufferData, binFileBufferLen);
		
        if (binFileBufferData == nullptr) {
            ERROR_LOG("malloc binFileBufferData failed");
            binFile.close();
            return nullptr;
        }
    } else {
		// 申请内存
        ret = aclrtMalloc(&binFileBufferData, binFileBufferLen, ACL_MEM_MALLOC_NORMAL_ONLY);//申请普通内存页
        if (ret != ACL_ERROR_NONE) {
            ERROR_LOG("malloc device buffer failed. size is %u", binFileBufferLen);
            binFile.close();
            return nullptr;
        }
    }
	// 将文件的内容读取至内存
    binFile.read(static_cast<char *>(binFileBufferData), binFileBufferLen);
    binFile.close();
    fileSize = binFileBufferLen;
    return binFileBufferData;
}

// 将文件的内容读取至内存 并且返回指针和大小
void* Utils::GetDeviceBufferOfFile(std::string fileName, uint32_t &fileSize)
{
	// 获得文件的大小和内容
    uint32_t inputHostBuffSize = 0;
    void* inputHostBuff = Utils::ReadBinFile(fileName, inputHostBuffSize);
	
    if (inputHostBuff == nullptr) {
        return nullptr;
    }
	 
	// 如果设备的 runMode ！= ACL_DEVICE 会直接将文件内容读取至设备内存 
    if (!g_isDevice) {
        void *inBufferDev = nullptr;
        uint32_t inBufferSize = inputHostBuffSize;
        aclError ret = aclrtMalloc(&inBufferDev, inBufferSize, ACL_MEM_MALLOC_NORMAL_ONLY);//申请普通内存页
        if (ret != ACL_ERROR_NONE) {
            ERROR_LOG("malloc device buffer failed. size is %u", inBufferSize);
            aclrtFreeHost(inputHostBuff);
            return nullptr;
        }
        //将HOST侧内存拷贝到device
        ret = aclrtMemcpy(inBufferDev, inBufferSize, inputHostBuff, inputHostBuffSize, ACL_MEMCPY_HOST_TO_DEVICE);
        if (ret != ACL_ERROR_NONE) {
            ERROR_LOG("memcpy failed. device buffer size is %u, input host buffer size is %u",
                inBufferSize, inputHostBuffSize);
            aclrtFree(inBufferDev);
            aclrtFreeHost(inputHostBuff);
            return nullptr;
        }
        aclrtFreeHost(inputHostBuff);
        fileSize = inBufferSize;
        return inBufferDev;
    } else {
        fileSize = inputHostBuffSize;
        return inputHostBuff;
    }
	// 结果就是  如果是acl_mode  会直接将文件内容读取至设备内存   
	// 如果不是  先将文件内容读取到host内存 再拷贝至设备
}
