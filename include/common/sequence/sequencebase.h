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
#ifndef TRAVODDS_COMMON_SEQUENCE_SEQUENCEBASE_H
#define TRAVODDS_COMMON_SEQUENCE_SEQUENCEBASE_H

#include <stdint.h>
#include <type_traits>
#include <memory>
#include <vector>
#include <cstddef>
#include "travodds_dll.h"

/**
 * @brief SequenceBase类,Sequence序列基类。
 */
class DDS_DLL SequenceBase {
public:
	/**
	 * @brief 获取队列的最大可存储空间大小。
	 *
	 * @return 最大可存储空间大小值。
	 */
	uint32_t maximum() const;

	/**
	 * @brief 获取序列下标对应的元素。
	 *
	 * @param [in] i 序列元素下标。
	 * 
	 * @return 下标对应的序列元素。
	 */
	void* operator [](uint32_t i);

	/**
	 * @brief 获取序列存储空间句柄。
	 *
	 * @return 序列存储空间句柄。
	 */
	void** buffer() const;

	// A borrowed sequence owns its pointer array and a pin, never the samples.
	bool has_ownership() const { return maximum_ != 0 && !loan_pin_; }
	bool has_loan() const { return static_cast<bool>(loan_pin_); }
	virtual size_t element_size() const { return 0; }
	virtual size_t element_alignment() const { return 0; }
	bool borrow(std::vector<void*> elements, std::shared_ptr<void> pin,
		std::shared_ptr<const void> owner, uint32_t role = 0);
	bool matches_loan(const SequenceBase& other, const std::shared_ptr<const void>& owner,
		uint32_t ownRole, uint32_t otherRole) const;

	/**
	 * @brief 获取序列当前长度。
	 *
	 * @return 序列当前长度。
	 */
	virtual uint32_t length() const = 0;

	/**
	 * @brief 设置序列最大长度。
	 *
	 * @param [in] newLength 序列新的最大长度。
	 */
	virtual bool length(uint32_t newLength) = 0;

	/**
	 * @brief 序列空间释放。
	 */
	virtual void unloan() = 0;

	/**
	 * @brief 析构函数。
	 */
	virtual ~SequenceBase();

protected:
	/**
	 * @brief 构造函数。
	 */
	SequenceBase() = default;

	uint32_t maximum_ = 0;
	uint32_t length_ = 0;
	void** elements_ = nullptr;
	std::vector<void*> loan_elements_;
	std::shared_ptr<void> loan_pin_;
	std::shared_ptr<const void> loan_owner_;
	uint32_t loan_role_ = 0;
	void clear_loan();
};

#endif
