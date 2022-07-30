/**
* @file acl.h
*
* Copyright (C) Huawei Technologies Co., Ltd. 2019-2020. All Rights Reserved.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
*/

#ifndef INC_EXTERNAL_ACL_ACL_H_
#define INC_EXTERNAL_ACL_ACL_H_

#include "acl_rt.h"
#include "acl_op.h"
#include "acl_mdl.h"

#ifdef __cplusplus
extern "C" {
#endif

// Current version is 1.1.0
#define ACL_MAJOR_VERSION    1
#define ACL_MINOR_VERSION    1
#define ACL_PATCH_VERSION    0

/**
 * @ingroup AscendCL
 * @brief acl initialize
 *
 * @par Restriction
 * aclInit接口在一个进程中只能调用一次
 * @param configPath [IN]  配置路径，可以为NULL
 * @retval ACL_SUCCESS 表示程序执行成功.
 * @retval OtherValues 代表程序执行失败
 */
ACL_FUNC_VISIBILITY aclError aclInit(const char *configPath);

/**
 * @ingroup AscendCL
 * @brief acl finalize
 *
 * @par Restriction
 * 需要在进程退出前调用aclFinalize
 * 调用aclFinalize后，服务将无法正常使用。
 * @retval ACL_SUCCESS 表示程序执行成功.
 * @retval OtherValues 代表程序执行失败
 */
ACL_FUNC_VISIBILITY aclError aclFinalize();

/**
 * @ingroup AscendCL
 * @brief query ACL interface version
 *
 * @param majorVersion[OUT] ACL 接口主板本
 * @param minorVersion[OUT] ACL 接口小版本
 * @param patchVersion[OUT] ACL 接口补丁版本
 * @retval ACL_SUCCESS 表示程序执行成功.
 * @retval OtherValues 代表程序执行失败
 */
ACL_FUNC_VISIBILITY aclError aclrtGetVersion(int32_t *majorVersion, int32_t *minorVersion, int32_t *patchVersion);

/**
 * @ingroup AscendCL
 * @brief get recent error message
 *
 * @retval null for failed
 * @retval OtherValues success
*/
ACL_FUNC_VISIBILITY const char *aclGetRecentErrMsg();

#ifdef __cplusplus
}
#endif

#endif // INC_EXTERNAL_ACL_ACL_H_