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
#pragma once

#include "AGIDDS_DLL.h"
#include "RetCode.h"
#include "BasicType.h"
#include "Status.h"
AGIDDS_NAMESPACE_BEGIN

class TypeObject;

class SerializedBuffer
{
public:
	/** @brief 有效数据首地址。 */
	char* buffer;
	/** @brief 缓冲区大小。 */
	unsigned int buffer_size;
	/** @brief 已写入位置。 */
	unsigned int writer_position;
	/** @brief 已读取位置。 */
	unsigned int reader_position;
	/** @brief 下一个缓冲区。 */
	SerializedBuffer* next;

	SerializedBuffer() {
		buffer = nullptr;
		buffer_size = 0;
		next = nullptr;
	}
};

typedef SequenceData<SerializedBuffer> SerializedBufferSeq;

class DDS_DLL TypeSupport {
public:
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
	 * @return int 表示复制操作结果的状态码，0表示成功，其他表示失败。
	 */
	virtual int copy_data(void* dst, void* src) = 0;

	/**
	 * @brief 获取指定样本数据序列化所需空间函数。
	 * @param data 指向要序列化的数据类型的指针。
	 * @return unsigned int 表示序列化该数据所需空间。
	 */
	virtual unsigned int get_serialized_data_size(void* data) = 0;

	/**
	 * @brief 获取序列化该类型样本最大所需空间。
	 * @return unsigned int 表示序列化该数据所需空间。
	 */
	virtual unsigned int get_max_serialized_data_size() = 0;

	/**
	 * @brief 序列化数据的函数。
	 * @param data 指向要序列化的数据类型的指针。
	 * @param buffer 指向包含序列化数据缓冲区的指针，要求已经提供足够的空间。
	 * @param endian 报文字节序 0：大端字节序 1 ： 小端字节序
	 * @return int 表示序列化操作成功的状态码。
	 */
	virtual int serialize_data(const void* data, SerializedBuffer* buffer, int endian) = 0;

	/**
	 * @brief 反序列化数据的函数。
	 * @param buffer 指向包含序列化数据的缓冲区的指针
	 * @param endian 报文字节序 0：大端字节序 1 ： 小端字节序
	 * @return void* 指向反序列化的数据类型的指针。
	 */
	 //virtual void* deserialize_data(SerializedBuffer* buffer, int buffer_size,int endian) = 0;

	virtual int deserialize_data(SerializedBuffer* buffer, void* data, int endian) = 0;

	/**
	 * @brief 获取数据类型的类型对象的函数。
	 * @return void* 指向数据类型的类型对象的指针。
	 */
	virtual TypeObject* get_typeobject() = 0;

	/* instance */
	virtual int MakeKey(const void*  data, InstanceHandle_t& iHandle, bool forceMd5 = false) = 0;

	virtual bool IsKey() { return isKey_; }


	static int IsLittleEndian()
	{
		int num = 1;
		// 将int指针转换为char指针，以便访问第一个字节
		char* byte = reinterpret_cast<char*>(&num);
		// 如果第一个字节是1，说明是小端
		return byte[0];
	}

protected:
	bool isKey_ = false;
};
AGIDDS_NAMESPACE_END

