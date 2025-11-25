#ifndef TypeSupport_h__
#define TypeSupport_h__

#include "travodds_dll.h"
#include "RetCode.h"

TRAVODDS_NAMESPACE_BEGIN

class TypeObject;
class SerializedBuffer;

/**
 * @brief SerializedBuffer 用于存储序列化数据的缓冲区，采用链表的形式以优化大包分片数据的性能。
 */
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
};

/**
 * @brief TypeSupport是一个抽象类，用于存储数据类型支持的纯虚函数，用于底层实现需要的时候回调。
 */
class TypeSupport
{
public:
    virtual ~TypeSupport() {}

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
     * @return int 表示序列化操作成功的状态码。
     */
    virtual int serialize_data(void* data, SerializedBuffer* buffer) = 0;

    /**
     * @brief 反序列化数据的函数。
     * @param buffer 指向包含序列化数据的缓冲区的指针。
     * @return void* 指向反序列化的数据类型的指针。
     */
    virtual void* deserialize_data(SerializedBuffer* buffer, int buffer_size) = 0;

    /**
     * @brief 获取数据类型的类型对象的函数。
     * @return void* 指向数据类型的类型对象的指针。
     */
    virtual TypeObject* get_typeobject() = 0;
};

class DomainParticipant
{
public:
    // 增加类型注册相关函数

    /**
     * @brief 向域参与者注册数据类型。
     * 
     * @param type_name 数据类型名称。
     * @param type_support 数据类型支持对象。
     * 
     * @return ReturnCode_t 返回注册结果。
     */ 
    virtual ReturnCode_t register_type(const char* type_name, const TypeSupport* type_support) = 0;

    /**
     * @brief 从域参与者注销数据类型。
     * 
     * @param type_name 数据类型名称。
     * 
     * @return ReturnCode_t 返回注销结果。
     */
    virtual ReturnCode_t unregister_type(const char* type_name) = 0;
};

TRAVODDS_NAMESPACE_END

#endif // TypeSupport_h__