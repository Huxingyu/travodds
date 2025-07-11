//Copyright(c)[2025][AGIROS][TravoDDS] is licensed under Mulan PSL v2.
//
//You can use this software according to the terms and conditions of
//the Mulan PSL v2.You may obtain a copy of Mulan PSL v2 at :
//http://license.coscl.org.cn/MulanPSL2
//
//THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF
//ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//NON-INFRINGEMENT, MERCHANTABILITY OR FIT FOR A PARTICULAR PURPOSE.
//
//See the Mulan PSL v2 for more details.
#ifndef TRAVODDS_DCPS_LISTENER_LISTENER_H
#define TRAVODDS_DCPS_LISTENER_LISTENER_H

#include "agidds_dll.h"

TRAVODDS_NAMESPACE_BEGIN

/**
 * @brief Listener类为所有回调类的基类, 最终用户应该使用它来实现对某些函数的特定回调。
 */
class DDS_DLL Listener {
public:
	/**
	 * @brief 构造函数。
	 */
	Listener() = default;

	/**
	 * @brief 析构函数。
	 */
    virtual ~Listener() = default;
};

TRAVODDS_NAMESPACE_END
#endif // !TRAVODDS_DCPS_LISTENER_LISTENER_H