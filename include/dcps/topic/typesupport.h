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
#ifndef TRAVODDS_DCPS_TOPIC_TYPESUPPORT_H
#define TRAVODDS_DCPS_TOPIC_TYPESUPPORT_H

#include "travodds_dll.h"
#include "type/basictype.h"
#include "dcps/base/retcode.h"
#include "dcps/base/status.h"
#include "cdr/travoddscdr.h"
#include "type/nativetypeinfo.h"
#include <cstddef>
#include <limits>
#include <new>
#include <type_traits>

TRAVODDS_NAMESPACE_BEGIN

class TypeObject;
class DynamicType;

//class DDS_DLL TypeObject {
//public:
//	virtual const std::pair<std::string, uint8_t>& get_type_descriptor() = 0;
//	virtual const std::vector<std::tuple<std::string, uint8_t, std::vector<int32_t>>>& get_members() = 0;
//};

typedef SequenceData<SerializedBuffer> SerializedBufferSeq;


class DDS_DLL TypeSupport {
public:
	virtual ~TypeSupport() = default;
	// Legacy/generated supports remain conservative until explicitly opted in.
	virtual NativeTypeInfo get_native_type_info() const { return {}; }
	virtual bool construct_native_sample(void*) const { return false; }
	/**
	 * @brief 创建样本的函数。
	 * @return void* 指向新创建的数据类型的指针。
	 */
	virtual void* create_data() = 0;

	/**
	 * @brief 删除样本的函数。
	 * @param data 指向要删除的数据类型的指针。
	 */
	virtual void delete_data(void* data) = 0;

	/**
	 * @brief 复制样本的函数。
	 * @param dst 指向目标数据类型的指针。
	 * @param src 指向源数据类型的指针。
	 * @return int32_t 表示复制操作结果的状态码，0表示成功，其他表示失败。
	 */
	virtual int32_t copy_data(void* dst, void* src) = 0;

	/**
	 * @brief 获取指定样本数据序列化所需空间函数。
	 * @param data 指向要序列化的数据类型的指针。
	 * @return uint32_t 表示序列化该数据所需空间。
	 */
	virtual uint32_t get_serialized_data_size(void* data, uint32_t currentAlignment) = 0;

	/**
	 * @brief 获取序列化该类型样本最大所需空间。
	 * @return uint32_t 表示序列化该数据所需空间。
	 */
	virtual uint32_t get_max_serialized_data_size(void* data, uint32_t currentAlignment) = 0;

	/**
	 * @brief 序列化数据的函数。
	 * @param data 指向要序列化的数据类型的指针。
	 * @param buffer 指向包含序列化数据缓冲区的指针，要求已经提供足够的空间。
	 * @param endian 报文字节序 0：大端字节序 1 ： 小端字节序
	 * @return int32_t 表示序列化操作成功的状态码。
	 */
	virtual int32_t serialize_data(void* data, CdrSerializer* cdr, int32_t endian) = 0;

	/**
	 * @brief 反序列化数据的函数。
	 * @param buffer 指向包含序列化数据的缓冲区的指针
	 * @param endian 报文字节序 0：大端字节序 1 ： 小端字节序
	 * @return void* 指向反序列化的数据类型的指针。
	 */
	virtual int32_t deserialize_data(void* data, CdrDeserializer* cdr, int32_t endian) = 0;

	/**
	 * @brief 获取数据类型的类型对象的函数。
	 * @return void* 指向数据类型的类型对象的指针。
	 */
	virtual TypeObject* get_typeobject() = 0;

	/**
	 * @brief 获取数据实例标识。
	 *
	 * @param data 指向数据类型的指针。
	 * @param cdr 指向Cdr序列化类对象，该对象已经关联了包含足够空间的用于序列化数据的缓冲区。
	 * @param [in,out]	iHandle 出口参数数据实例标识。
	 * @param forceMd5 是否强制使用MD5。
	 *
	 * @return ReturnCode_t 表示操作结果的状态码。
	 */
	virtual ReturnCode_t get_instancehandle(void* data, CdrSerializer* cdr, InstanceHandle_t& iHandle, bool forceMd5 = false) = 0;


	virtual DynamicType* get_dynamictype() { return nullptr; };

	/**
	 * @brief 数据类型是否包含key。
	 *
	 * @return bool 如果数据类型包含key返回true，否则返回false。
	 */
	virtual bool has_key() = 0;

	/**
	 * @brief 获取数据类型名称。
	 *
	 * @return const char* 数据类型名称。
	 */
	virtual const char* get_typename() = 0;

	/* instance */
	virtual int32_t MakeKey(const void*  data, InstanceHandle_t& iHandle, bool forceMd5 = false) = 0;

	static int32_t IsLittleEndian()
	{
		int32_t num = 1;
		// 将int32_t指针转换为char指针，以便访问第一个字节
		char* byte = reinterpret_cast<char*>(&num);
		// 如果第一个字节是1，说明是小端
		return byte[0];
	}
};

// Implement serializers/key operations in a derived support as usual. The
// application supplies a versioned layout ID, and must ensure pointer-free T.
template<typename T>
class NativeTypeSupport : public TypeSupport {
public:
	explicit NativeTypeSupport(std::string layoutId) : layout_id_(std::move(layoutId)) {
		static_assert(std::is_standard_layout<T>::value && std::is_trivially_copyable<T>::value &&
			std::is_trivially_destructible<T>::value && std::is_nothrow_default_constructible<T>::value,
			"Native samples must have a trivial, fixed, noexcept constructible layout");
		static_assert(sizeof(T) <= std::numeric_limits<uint32_t>::max(), "Native sample too large");
	}
	NativeTypeInfo get_native_type_info() const override {
		return {true, true, uint32_t(sizeof(T)), uint32_t(alignof(T)), layout_id_};
	}
	bool construct_native_sample(void* storage) const override {
		if (!storage || reinterpret_cast<uintptr_t>(storage) % alignof(T)) return false;
		new (storage) T{};
		return true;
	}
private:
	std::string layout_id_;
};
TRAVODDS_NAMESPACE_END

#endif // ! TRAVODDS_DCPS_TOPIC_TYPESUPPORT_H
